#pragma once

#include <memory>
#include <queue>
#include <functional>
#include <optional>
#include <cassert>
#include <chrono>

#include "../common/Utils.h"
#include "../common/Constants.h"
#include "../common/Logging.h"

#include "../networking/tcp/TCPConnectionInterface.h"
#include "../networking/tcp/TCPMessage.h"
#include "../networking/udp/UDPTransportInterface.h"
#include "../networking/udp/UDPMessage.h"

#include "Message.h"

class NetworkClient
{
    public:
        struct Config
        {
            asio::ip::udp::endpoint serverUDPEndpoint;

            int udpBindRequestTimeoutMs = 3000;
            int udpBindRequestPollMs    = 50;

            int udpBindAckTimeoutMs     = 3000;
            int udpBindAckPollMs        = 50;

            bool enablePing = true;
            std::chrono::milliseconds pingInterval{1000};
            Constants::TransportType pingTransport = Constants::TransportType::TCP;
        };

    private:
        struct PendingPing
        {
            std::chrono::steady_clock::time_point timePingSent;
            uint32_t seq = 0;
        };

    public:
        NetworkClient(std::shared_ptr<TCPConnectionInterface> tcpConnection,
                      std::shared_ptr<UDPTransportInterface> udpTransport,
                      Config config)
        : m_tcpConnection(tcpConnection)
        , m_udpTransport(udpTransport)
        , m_config(config)
        {
            assert(m_tcpConnection != nullptr);
            assert(m_udpTransport != nullptr);
            assert(m_tcpConnection->isOpen());
            assert(m_udpTransport->isOpen());
        }

        bool doUDPHandshake()
        {
            LOG_INFO("Starting NetworkClient UDP handshake");

            UDPBindRequest udpBindRequest;
            bool gotBindRequest = waitForUDPBindRequest(udpBindRequest);

            if (!gotBindRequest)
            {
                LOG_ERROR("Timed out waiting for UDPBindRequest");
                return false;
            }

            LOG_INFO("Received UDPBindRequest (token=%d)", udpBindRequest.token);

            bool gotBindAck = sendUDPBindAndWaitForAck(udpBindRequest.token);

            if (!gotBindAck)
            {
                LOG_ERROR("Timed out waiting for UDPBindAck (token=%d)",
                          udpBindRequest.token);
                return false;
            }

            m_doneUDPHandshake = true;

            LOG_INFO("UDP handshake complete");
            return true;
        }

        bool isConnected() const
        {
            return m_udpTransport->isOpen() && m_tcpConnection->isOpen()
                && m_doneUDPHandshake;
        }

        void pumpReceive()
        {
            TCPMessage tcpMessage;
            while (m_tcpConnection->tryRecv(tcpMessage))
            {
                dispatchIncomingMessage(fromTCPMessage(tcpMessage));
            }

            UDPMessage udpMessage;
            asio::ip::udp::endpoint sender;
            while (m_udpTransport->tryRecv(udpMessage, sender))
            {
                if (sender == m_config.serverUDPEndpoint)
                {
                    dispatchIncomingMessage(fromUDPMessage(udpMessage));
                }
                else
                {
                    LOG_WARNING("Received UDP message from unknown sender %s:%u",
                                sender.address().to_string().c_str(),
                                sender.port());
                }
            }
        }

        std::vector<Message> consumeIncomingMessages()
        {
            std::vector<Message> vec(m_inbox);
            m_inbox.clear();

            return vec;
        }

        void queueOutgoingMessage(const Message& message, Constants::TransportType transport)
        {
            switch (transport)
            {
                case Constants::TransportType::TCP:
                    m_tcpOutbox.push_back(toTCPMessage(message));
                    break;
                case Constants::TransportType::UDP:
                    m_udpOutbox.push_back(toUDPMessage(message));
                    break;
                default:
                    throw std::runtime_error("Unhandled transport type");
            }
        }

        void pumpSend()
        {
            maybeQueuePing();

            for (const auto& tcpMessage : m_tcpOutbox)
            {
                sendTCPMessage(tcpMessage);
            }
            for (const auto& udpMessage : m_udpOutbox)
            {
                sendUDPMessage(udpMessage);
            }

            m_tcpOutbox.clear();
            m_udpOutbox.clear();
        }

        std::optional<float> getPingMs() const
        {
            return m_pingMs;
        }

    private:
        bool waitForUDPBindRequest(UDPBindRequest& udpBindRequest)
        {
            int elapsed = 0;

            while (elapsed < m_config.udpBindRequestTimeoutMs)
            {
                TCPMessage tcpMessage;
                Message message;

                while (m_tcpConnection->tryRecv(tcpMessage))
                {
                    message = fromTCPMessage(tcpMessage);
                    if (message.type == MessageType::UDPBindRequest)
                    {
                        udpBindRequest = message.data.udpBindRequest;
                        return true;
                    }
                }

                sleepMs(m_config.udpBindRequestPollMs);
                elapsed += m_config.udpBindRequestPollMs;
            }

            return false;
        }

        bool sendUDPBindAndWaitForAck(int token)
        {
            int elapsed = 0;

            Message udpBind{
                .type = MessageType::UDPBind,
                .data = { .udpBind = { .token = token } }
            };

            while (elapsed < m_config.udpBindAckTimeoutMs)
            {
                TCPMessage tcpMessage;
                Message message;

                while (m_tcpConnection->tryRecv(tcpMessage))
                {
                    message = fromTCPMessage(tcpMessage);
                    if (message.type == MessageType::UDPBindAck)
                    {
                        return true;
                    }
                }

                sendUDPMessage(toUDPMessage(udpBind));

                sleepMs(m_config.udpBindAckPollMs);
                elapsed += m_config.udpBindAckPollMs;
            }

            return false;
        }

        void sendTCPMessage(const TCPMessage& tcpMessage)
        {
            if (canSendTCPMessage())
            {
                m_tcpConnection->send(tcpMessage);
            }
        }

        void sendUDPMessage(const UDPMessage& udpMessage)
        {
            if (canSendUDPMessage())
            {
                m_udpTransport->sendTo(udpMessage, m_config.serverUDPEndpoint);
            }
        }

        bool canSendTCPMessage() const
        {
            return m_tcpConnection->isOpen();
        }

        bool canSendUDPMessage() const
        {
            return m_udpTransport->isOpen();
        }

        void dispatchIncomingMessage(const Message& message)
        {
            if (message.type == MessageType::Pong)
            {
                handlePong(message.data.pong);
            }
            else
            {
                m_inbox.push_back(message);
            }
        }

        void maybeQueuePing()
        {
            if (!m_config.enablePing)
            {
                return;
            }

            auto now = std::chrono::steady_clock::now();

            if (isPingInProgress(now))
            {
                return;
            }

            m_pendingPing = PendingPing{now, m_nextPingSeq++};
            Message pingMsg{
                .type = MessageType::Ping,
                .data = { .ping = { .seq = m_pendingPing.value().seq } }
            };
            queueOutgoingMessage(pingMsg, m_config.pingTransport);
        }

        void handlePong(Pong pongMessage)
        {
            if (m_pendingPing && m_pendingPing.value().seq == pongMessage.seq)
            {
                auto now = std::chrono::steady_clock::now();
                auto timeBetween = now - m_pendingPing.value().timePingSent;
                m_pingMs = std::chrono::duration<float, std::milli>(timeBetween).count();
            }
        }

        bool isPingInProgress(const auto& now) const
        {
            return m_pendingPing
                && (now - m_pendingPing.value().timePingSent) < m_config.pingInterval;
        }

    private:
        std::shared_ptr<TCPConnectionInterface> m_tcpConnection;
        std::shared_ptr<UDPTransportInterface> m_udpTransport;

        Config m_config;

        std::vector<Message> m_inbox;
        std::vector<TCPMessage> m_tcpOutbox;
        std::vector<UDPMessage> m_udpOutbox;

        bool m_doneUDPHandshake = false;

        std::optional<PendingPing> m_pendingPing;
        uint32_t m_nextPingSeq = 0;
        std::optional<float> m_pingMs;
};
