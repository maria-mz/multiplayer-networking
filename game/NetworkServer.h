#pragma once

#include <unordered_map>

#include "../common/Constants.h"
#include "../common/Logging.h"

#include "../networking/tcp/TCPListenerInterface.h"
#include "../networking/tcp/TCPConnectionInterface.h"
#include "../networking/tcp/TCPMessage.h"
#include "../networking/udp/UDPTransportInterface.h"
#include "../networking/udp/UDPMessage.h"

#include "Message.h"

// Hash specialization so udp endpoint can be used as a key in endpoint -> clientID lookup map
namespace std
{
    template<>
    struct hash<asio::ip::udp::endpoint>
    {
        size_t operator()(const asio::ip::udp::endpoint& ep) const noexcept
        {
            return std::hash<std::string>()(ep.address().to_string()) ^
                   std::hash<uint16_t>()(ep.port());
        }
    };
}

using ClientID = u_int32_t;

// Intended usage:
//
// Accept new clients
// acceptConnections()
//
// Receive messages over the network
// pumpReceive() (may trigger onClientConnect callback)
//
// Consume any incoming messages and do things
// ... consumeIncomingMessages(...)
//
// Push messages to send out in the next pump
// pushOutgoingMessage(...)
//
// Send messages over the network
// pumpSend()
//
// Clean up any disconnected clients (may trigger onClientDisconnect callback)
// cleanupDisconnectedClients(...)
//
// Repeat

class NetworkServer
{
    private:
        struct NetworkClient
        {
            enum class Status { Disconnected, BindingUDP, Connected };

            ClientID clientID;
            Status status = Status::Disconnected;
            int udpToken;

            std::shared_ptr<TCPConnectionInterface> tcpConnection;
            std::optional<asio::ip::udp::endpoint> udpEndpoint;

            std::vector<TCPMessage> tcpOutbox;
            std::vector<UDPMessage> udpOutbox;

            std::vector<Message> inbox;
        };

    public:
        NetworkServer(std::shared_ptr<TCPListenerInterface> tcpListener,
                      std::shared_ptr<UDPTransportInterface> udpTransport)
        : m_tcpListener(tcpListener)
        , m_udpTransport(udpTransport)
        {
            assert(m_tcpListener != nullptr);
            assert(m_udpTransport != nullptr);
        }

        void acceptConnections()
        {
            while (auto newTCPConnection = m_tcpListener->getNextConnection())
            {
                addNetworkClient(newTCPConnection);
            }
        }

        void queueOutgoingMessage(ClientID clientID,
                                 const Message& message,
                                 Constants::TransportType transportType)
        {
            auto it = m_clients.find(clientID);
            if (it == m_clients.end())
            {
                LOG_WARNING("Attempted to queue message for non-existent client %u",
                            clientID);
                return;
            }

            queueOutgoingMessage(it->second, message, transportType);
        }

        void queueBroadcast(const Message& message,
                            Constants::TransportType transportType,
                            std::optional<ClientID> ignoreClientID)
        {
            for (auto& [clientID, client] : m_clients)
            {
                if (!ignoreClientID || clientID != *ignoreClientID)
                {
                    queueOutgoingMessage(client, message, transportType);
                }
            }
        }

        void pumpSend()
        {
            for (auto& [clientID, client] : m_clients)
            {
                for (const auto& tcpMessage : client.tcpOutbox)
                {
                    sendTCPMessage(tcpMessage, client);
                }
                for (const auto& udpMessage : client.udpOutbox)
                {
                    sendUDPMessage(udpMessage, client);
                }

                client.tcpOutbox.clear();
                client.udpOutbox.clear();
            }
        }

        void pumpReceive()
        {
            for (auto& [clientID, client] : m_clients)
            {
                TCPMessage tcpMessage;
                while (client.tcpConnection->tryRecv(tcpMessage))
                {
                    client.inbox.push_back(fromTCPMessage(tcpMessage));
                }
            }

            UDPMessage udpMessage;
            Message message;
            asio::ip::udp::endpoint sender;

            while (m_udpTransport->tryRecv(udpMessage, sender))
            {
                message = fromUDPMessage(udpMessage);

                auto it = m_udpEndpointToClientID.find(sender);
                if (it != m_udpEndpointToClientID.end())
                {
                    m_clients[it->second].inbox.push_back(message);
                }
                else if (message.type == MessageType::UDPBind)
                {
                    handleUDPBind(message.data.udpBind, sender);
                }
                else
                {
                    LOG_WARNING("Received UDP message from unknown sender %s:%u",
                                sender.address().to_string().c_str(),
                                sender.port());
                }
            }
        }

        std::vector<Message> consumeIncomingMessages(ClientID clientID)
        {
            auto it = m_clients.find(clientID);
            if (it == m_clients.end())
            {
                LOG_WARNING("Attempted to consume messages for non-existent client %u",
                            clientID);
                return {};
            }

            NetworkClient& client = it->second;

            std::vector<Message> vec(client.inbox);
            client.inbox.clear();

            return vec;
        }

        void cleanupDisconnectedClients()
        {
            std::vector<ClientID> disconnectedClientIDs;

            if (!m_udpTransport->isOpen())
            {
                disconnectedClientIDs = getClientIDs();
            }
            else
            {
                for (const auto& [clientID, client] : m_clients)
                {
                    if (!client.tcpConnection->isOpen()
                        || client.status == NetworkClient::Status::Disconnected)
                    {
                        disconnectedClientIDs.push_back(clientID);
                    }
                }
            }

            for (const auto& clientID : disconnectedClientIDs)
            {
                removeNetworkClient(clientID);
                if (m_onClientDisconnect)
                {
                    m_onClientDisconnect(clientID);
                }
            }
        }

        void setOnClientConnect(std::function<void(ClientID)> callback)
        {
            m_onClientConnect = std::move(callback);
        }

        void setOnClientDisconnect(std::function<void(ClientID)> callback)
        {
            m_onClientDisconnect = std::move(callback);
        }

        std::vector<ClientID> getClientIDs()
        {
            std::vector<ClientID> clientIDs;
            clientIDs.reserve(m_clients.size());

            for (const auto& [clientID, client] : m_clients)
            {
                clientIDs.push_back(clientID);
            }

            return clientIDs;
        }

    private:
        void addNetworkClient(std::shared_ptr<TCPConnectionInterface> tcpConnection)
        {
            NetworkClient client;
            client.clientID = getNextClientID();
            client.tcpConnection = tcpConnection;
            client.tcpConnection->setID(client.clientID);
            client.udpToken = (int)(client.clientID) + 100;

            m_clients[client.clientID] = client;
            LOG_INFO("Created client %d on the network", client.clientID);

            initiateUDPBind(m_clients.at(client.clientID));
        }

        void removeNetworkClient(ClientID clientID)
        {
            auto it = m_clients.find(clientID);
            if (it == m_clients.end())
            {
                return;
            }

            NetworkClient& client = it->second;
            if (client.udpEndpoint)
            {
                m_udpEndpointToClientID.erase(*client.udpEndpoint);
            }

            m_clients.erase(clientID);
            LOG_INFO("Removed client %d from the network", clientID);
        }

        void initiateUDPBind(NetworkClient& client)
        {
            LOG_INFO("Initiating UDP bind for client %d", client.clientID);

            client.status = NetworkClient::Status::BindingUDP;
            Message bindRequest{
                .type = MessageType::UDPBindRequest,
                .data = { .udpBindRequest= { .token = client.udpToken } }
            };

            client.tcpOutbox.push_back(toTCPMessage(bindRequest));
        }

        void handleUDPBind(const UDPBind& udpBindMessage,
                           const asio::ip::udp::endpoint& sender)
        {
            auto it = std::find_if(
                m_clients.begin(),
                m_clients.end(),
                [&](const auto& pair)
                {
                    return pair.second.udpToken == udpBindMessage.token;
                });

            if (it == m_clients.end())
            {
                LOG_WARNING("UDP bind failed: no matching client for token %d from %s:%u",
                            udpBindMessage.token,
                            sender.address().to_string().c_str(),
                            sender.port());
                return;
            }

            NetworkClient& client = it->second;

            client.udpEndpoint = sender;
            client.status = NetworkClient::Status::Connected;
            m_udpEndpointToClientID[sender] = client.clientID;

            LOG_INFO("UDP bound client %d to %s:%u",
                     client.clientID,
                     sender.address().to_string().c_str(),
                     sender.port());

            Message bindAck{
                .type = MessageType::UDPBindAck,
                .data = { .udpBindAck = { } }
            };
            client.tcpOutbox.push_back(toTCPMessage(bindAck));

            if (m_onClientConnect)
            {
                m_onClientConnect(client.clientID);
            }
        }

        void sendTCPMessage(const TCPMessage& tcpMessage, NetworkClient& client)
        {
            if (canSendTCPMessage(client))
            {
                client.tcpConnection->send(tcpMessage);
            }
        }

        void sendUDPMessage(const UDPMessage& udpMessage, NetworkClient& client)
        {
            if (canSendUDPMessage(client))
            {
                assert(client.udpEndpoint);
                m_udpTransport->sendTo(udpMessage, *(client.udpEndpoint));
            }
        }

        bool canSendTCPMessage(NetworkClient& client) const
        {
            return client.status != NetworkClient::Status::Disconnected
                && client.tcpConnection->isOpen();
        }

        bool canSendUDPMessage(NetworkClient& client) const
        {
            return client.status == NetworkClient::Status::Connected
                && m_udpTransport->isOpen();
        }

        void queueOutgoingMessage(NetworkClient& client,
                                  const Message& message,
                                  Constants::TransportType transportType)
        {
            switch (transportType)
            {
                case Constants::TransportType::TCP:
                    client.tcpOutbox.push_back(toTCPMessage(message));
                    break;
                case Constants::TransportType::UDP:
                    client.udpOutbox.push_back(toUDPMessage(message));
                    break;
                default:
                    throw std::runtime_error("Unhandled transport type");
            }
        }

        ClientID getNextClientID() { return m_nextClientID++; }

    private:
        std::shared_ptr<TCPListenerInterface> m_tcpListener;
        std::shared_ptr<UDPTransportInterface> m_udpTransport;

        std::unordered_map<ClientID, NetworkClient> m_clients;

        ClientID m_nextClientID = 10000;

        std::unordered_map<asio::ip::udp::endpoint, ClientID> m_udpEndpointToClientID;

        std::function<void(ClientID)> m_onClientConnect = nullptr;
        std::function<void(ClientID)> m_onClientDisconnect = nullptr;
};
