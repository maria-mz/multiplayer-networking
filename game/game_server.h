#pragma once

#include <unordered_map>

#include "common/constants.h"

#include "networking/tcp/tcp_message.h"
#include "networking/udp/udp_message.h"

#include "network_server.h"

class GameServer
{
    public:
        struct Config
        {
            constants::TransportType transportForPlayerStateUpdates = constants::TransportType::TCP;
        };

    public:
        GameServer(Config config, std::shared_ptr<NetworkServer> networkServer)
        : m_config(config)
        , m_networkServer(networkServer)
        {
            m_networkServer->setOnClientConnect([this](ClientID clientID) {
                onClientConnected(clientID);
            });
            m_networkServer->setOnClientDisconnect([this](ClientID clientID) {
                onClientDisconnected(clientID);
            });
        }

        void update()
        {
            m_networkServer->acceptConnections();
            m_networkServer->pumpReceive();

            // Relay PlayerState updates from one player to every player,
            // for all players.
            for (const auto& clientID : m_networkServer->getClientIDs())
            {
                auto messages = m_networkServer->consumeIncomingMessages(clientID);
                for (const auto& message : messages)
                {
                    if (message.type == MessageType::PlayerStateUpdate
                        || message.type == MessageType::ProjectileSpawn)
                    {
                        m_networkServer->queueBroadcast(
                            message, m_config.transportForPlayerStateUpdates, clientID
                        );
                    }
                }
            }

            m_networkServer->pumpSend();
            m_networkServer->cleanupDisconnectedClients();
        }

    private:
        void onClientConnected(ClientID connectedClientID)
        {
            uint playerID = getNextPlayerID();

            Message assignPlayerID{
                MessageType::AssignLocalPlayerID,
                {
                    .assignLocalPlayerID = {
                        playerID
                    }
                }
            };

            Message playerJoined{
                MessageType::PlayerJoined,
                {
                    .playerJoined = {
                        playerID
                    }
                }
            };

            m_clientIDToPlayerID[connectedClientID] = playerID;

            m_networkServer->queueOutgoingMessage(
                connectedClientID, assignPlayerID, constants::TransportType::TCP
            );
            m_networkServer->queueBroadcast(
                assignPlayerID, constants::TransportType::TCP, connectedClientID
            );
        };

        void onClientDisconnected(ClientID disconnectedClientID)
        {
            uint playerID = m_clientIDToPlayerID[disconnectedClientID];

            Message playerLeftMsg{
                MessageType::PlayerLeft,
                {
                    .playerLeft = {
                        playerID
                    }
                }
            };

            m_networkServer->queueBroadcast(
                playerLeftMsg, constants::TransportType::TCP, disconnectedClientID
            );
        };

        uint getNextPlayerID() { return m_nextPlayerID++; }

    private:
        std::shared_ptr<NetworkServer> m_networkServer;
        Config m_config;

        uint m_nextPlayerID = 0;
        std::unordered_map<ClientID, uint> m_clientIDToPlayerID;
};
