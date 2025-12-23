#pragma once

#include <unordered_map>
#include <format>
#include <cassert>

#include "../common/Logging.h"

#include "Player.h"
#include "GameMessage.h"

using PlayerID = uint;

class GameSimulation
{
    public:
        void addPlayer(PlayerID playerID)
        {
            if (isPlayerInGame(playerID))
            {
                LOG_WARNING("Attempted to add player %u, but they are already in the game",
                            playerID);
                return;
            }

            m_players[playerID] = std::make_unique<Player>();
            LOG_INFO("Added player %u to the game", playerID);
        }

        void addLocalPlayer(PlayerID localPlayerID)
        {
            addPlayer(localPlayerID);
            setLocalPlayerID(localPlayerID);
        }

        void removePlayer(PlayerID playerID)
        {
            if (!isPlayerInGame(playerID))
            {
                LOG_WARNING("Attempted to remove player %u, but they are already not in the game",
                            playerID);
                return;
            }

            m_players.erase(playerID);
            LOG_INFO("Removed player %u from the game", playerID);

            if (playerID == m_localPlayerID)
            {
                m_localPlayerID = std::nullopt;
            }
        }

        std::optional<PlayerID> getLocalPlayerID()
        {
            return m_localPlayerID;
        }

        void setLocalPlayerID(std::optional<PlayerID> localPlayerID)
        {
            if (localPlayerID && !isPlayerInGame(*localPlayerID))
            {
                throw std::invalid_argument(
                    std::format("Cannot set local player ID to {}: player must be in the game",
                                *localPlayerID
                    )
                );
            }
            m_localPlayerID = localPlayerID;
        }

        void applyLocalInput(GameEvent event)
        {
            std::unique_ptr<Player>& player = getLocalPlayer();

            if (event != GameEvent::None)
            {
                player->input(event);
            }
        }

        void applyIncomingMessages(const std::vector<GameMessage>& inMessages)
        {
            for (const auto& msg : inMessages)
            {
                switch (msg.type)
                {
                    case GameMessageType::PlayerJoined:
                        handlePlayerJoinedMessage(msg.data.playerJoined);
                        break;
                    case GameMessageType::PlayerLeft:
                        handlePlayerLeftMessage(msg.data.playerLeft);
                        break;
                    case GameMessageType::PlayerStateUpdate:
                        handlePlayerStateUpdateMessage(msg.data.playerStateUpdate);
                        break;
                    default:
                        break;
                }
            }
        }

        std::vector<GameMessage> collectOutgoingMessages()
        {
            if (!m_localPlayerID)
            {
                return {};
            }

            std::unique_ptr<Player>& localPlayer = getLocalPlayer();

            GameMessage localStateUpdateMsg;
            localStateUpdateMsg.type = GameMessageType::PlayerStateUpdate;
            localStateUpdateMsg.data.playerStateUpdate = {
                .posX = localPlayer->m_position.x,
                .posY = localPlayer->m_position.y,
                .velX = localPlayer->m_velocity.x,
                .velY = localPlayer->m_velocity.y,
                .state = localPlayer->getState(),
                .playerID = *m_localPlayerID
            };

            return std::vector<GameMessage>{localStateUpdateMsg};
        }

        void updateSimulation(int deltaTime)
        {
            if (m_localPlayerID)
            {
                std::unique_ptr<Player>& localPlayer = getLocalPlayer();
                localPlayer->update(deltaTime);
            }
        }

        const std::unordered_map<PlayerID, std::unique_ptr<Player>>& getPlayers()
        {
            return m_players;
        }

        bool isLocalPlayer(PlayerID playerID)
        {
            return m_localPlayerID && playerID == *m_localPlayerID;
        }

        bool isPlayerInGame(PlayerID playerID)
        {
            return m_players.contains(playerID);
        }

    private:
        void updateRemotePlayerState(PlayerID playerID, const PlayerStateUpdate& stateUpdate)
        {
            assert(!isLocalPlayer(playerID) && isPlayerInGame(playerID));

            std::unique_ptr<Player>& remotePlayer = m_players.at(playerID);

            remotePlayer->maybeChangeState(stateUpdate.state);
            remotePlayer->m_position.x = stateUpdate.posX;
            remotePlayer->m_position.y = stateUpdate.posY;
            remotePlayer->m_velocity.x = stateUpdate.velX;
            remotePlayer->m_velocity.y = stateUpdate.velY;

            LOG_DEBUG("Updated remote player %u state: pos=(%f, %f), vel=(%f, %f)",
                    playerID,
                    stateUpdate.posX, stateUpdate.posY,
                    stateUpdate.velX, stateUpdate.velY);
        }

        std::unique_ptr<Player>& getLocalPlayer()
        {
            assert(m_localPlayerID);
            assert(isPlayerInGame(*m_localPlayerID));
            std::unique_ptr<Player>& localPlayer = m_players.at(*m_localPlayerID);
            return localPlayer;
        }

        void handlePlayerJoinedMessage(PlayerJoined playerJoined)
        {
            addPlayer(playerJoined.playerID);
        }

        void handlePlayerLeftMessage(PlayerLeft playerLeft)
        {
            removePlayer(playerLeft.playerID);
        }

        void handlePlayerStateUpdateMessage(PlayerStateUpdate playerStateUpdate)
        {
            if (isLocalPlayer(playerStateUpdate.playerID))
            {
                // The local player shouldn't be receiving state updates if programmed correctly,
                // as its state is determined locally.
                LOG_WARNING(
                    "Received state update for local player %u. Ignoring.",
                    playerStateUpdate.playerID
                );
                return;
            }

            if (!isPlayerInGame(playerStateUpdate.playerID))
            {
                // Assuming no malicious actors, we just missed the player join message,
                // so adding the player now
                addPlayer(playerStateUpdate.playerID);
            }

            updateRemotePlayerState(playerStateUpdate.playerID,
                                    playerStateUpdate);
        }

    private:
        std::unordered_map<PlayerID, std::unique_ptr<Player>> m_players;
        std::optional<PlayerID> m_localPlayerID;
};
