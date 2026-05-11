#pragma once

#include <unordered_map>

#include "common/constants.h"
#include "common/logging.h"

#include "networking/tcp/tcp_message.h"
#include "networking/udp/udp_message.h"

#include "game_simulation.h"
#include "network_server.h"
#include "message.h"

class GameServer
{
    public:
        GameServer(std::shared_ptr<NetworkServer> networkServer)
        : m_networkServer(networkServer)
        , m_gameSimulation(GameSimulation::Config{ .projectileDamage = 25, .registerProjectileHits = true })
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
                            message, constants::TransportType::UDP, clientID
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

            auto outgoingMessages = m_gameSimulation.collectOutgoingMessages();
            for (const auto& outMsg : outgoingMessages)
            {
                m_networkServer->queueBroadcast(
                    outMsg, constants::TransportType::TCP, std::nullopt
                );
            }

            m_networkServer->pumpSend();
            m_networkServer->cleanupDisconnectedClients();
        }

    private:
        void onClientConnected(ClientID connectedClientID)
        {
            uint playerID = getNextPlayerID();

            m_networkServer->queueOutgoingMessage(
                connectedClientID,
                makeAssignLocalPlayerIDMessage(playerID),
                constants::TransportType::TCP
            );

            for (const auto& [playerID, player] : m_gameSimulation.getPlayers())
            {
                m_networkServer->queueOutgoingMessage(
                    connectedClientID,
                    makePlayerStateUpdateMessage(playerID, *player),
                    constants::TransportType::TCP
                );

                m_networkServer->queueOutgoingMessage(
                    connectedClientID,
                    makePlayerHealthUpdateMessage(playerID, player->getHealth()),
                    constants::TransportType::TCP
                );
            }

            m_clientIDToPlayerID[connectedClientID] = playerID;
            m_gameSimulation.addPlayer(playerID);

            m_networkServer->queueBroadcast(
                makePlayerJoinedMessage(playerID),
                constants::TransportType::TCP,
                connectedClientID
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

            m_networkServer->queueBroadcast(
                makePlayerLeftMessage(playerID),
                constants::TransportType::TCP,
                disconnectedClientID
            );
        };

        uint getNextPlayerID() { return m_nextPlayerID++; }

    private:
        std::shared_ptr<NetworkServer> m_networkServer;
        GameSimulation m_gameSimulation;

        uint m_nextPlayerID = 0;
        std::unordered_map<ClientID, uint> m_clientIDToPlayerID;
};
