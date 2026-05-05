#pragma once

#include <unordered_map>

#include "common/constants.h"
#include "common/logging.h"

#include "networking/tcp/tcp_message.h"
#include "networking/udp/udp_message.h"

#include "game_simulation.h"
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

        void update(int deltaTimeMs)
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
                    if (message.type == MessageType::PlayerStateUpdate)
                    {
                        m_gameSimulation.applyIncomingMessage(message);
                        m_networkServer->queueBroadcast(
                            message, m_config.transportForPlayerStateUpdates, clientID
                        );
                    }
                    else if (message.type == MessageType::ProjectileSpawn)
                    {
                        m_gameSimulation.applyIncomingMessage(message);
                        m_networkServer->queueBroadcast(
                            message, constants::TransportType::TCP, clientID
                        );
                    }
                }
            }

            m_gameSimulation.updateSimulation(deltaTimeMs);
            handleProjectileHits(m_gameSimulation.detectProjectileHits());

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
            m_gameSimulation.addPlayer(playerID);

            m_networkServer->queueOutgoingMessage(
                connectedClientID, assignPlayerID, constants::TransportType::TCP
            );
            m_networkServer->queueBroadcast(
                assignPlayerID, constants::TransportType::TCP, connectedClientID
            );
        };

        void onClientDisconnected(ClientID disconnectedClientID)
        {
            auto it = m_clientIDToPlayerID.find(disconnectedClientID);
            if (it == m_clientIDToPlayerID.end())
            {
                return;
            }

            uint playerID = it->second;
            m_gameSimulation.removePlayer(playerID);
            m_clientIDToPlayerID.erase(it);

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

        void handleProjectileHits(const std::vector<ProjectileHit>& hits)
        {
            std::vector<ProjectileID> hitProjectileIDs;
            hitProjectileIDs.reserve(hits.size());

            for (const auto& hit : hits)
            {
                LOG_INFO("Projectile %u from player %u hit player %u",
                         hit.projectileID,
                         hit.ownerPlayerID,
                         hit.hitPlayerID);
                hitProjectileIDs.push_back(hit.projectileID);
            }

            m_gameSimulation.removeProjectiles(hitProjectileIDs);
        }

    private:
        std::shared_ptr<NetworkServer> m_networkServer;
        GameSimulation m_gameSimulation;
        Config m_config;

        uint m_nextPlayerID = 0;
        std::unordered_map<ClientID, uint> m_clientIDToPlayerID;
};
