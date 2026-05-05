#pragma once

#include <map>
#include <format>
#include <cassert>
#include <vector>

#include "common/logging.h"
#include "common/constants.h"

#include "player.h"
#include "projectile.h"
#include "message.h"


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
            m_players[localPlayerID]->isLocal = true;
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

            if (event == GameEvent::Shoot_Pressed)
            {
                spawnLocalProjectile(*player);
                return;
            }

            if (event != GameEvent::None)
            {
                player->input(event);
            }
        }

        void applyIncomingMessage(const Message& message)
        {
            switch (message.type)
            {
                case MessageType::PlayerJoined:
                    handlePlayerJoinedMessage(message.data.playerJoined);
                    break;
                case MessageType::PlayerLeft:
                    handlePlayerLeftMessage(message.data.playerLeft);
                    break;
                case MessageType::PlayerStateUpdate:
                    handlePlayerStateUpdateMessage(message.data.playerStateUpdate);
                    break;
                case MessageType::ProjectileSpawn:
                    handleProjectileSpawnMessage(message.data.projectileSpawn);
                    break;
                default:
                    break;
            }
        }

        std::vector<Message> collectOutgoingMessages()
        {
            if (!m_localPlayerID)
            {
                return {};
            }

            std::unique_ptr<Player>& localPlayer = getLocalPlayer();

            Message localStateUpdateMsg;
            localStateUpdateMsg.type = MessageType::PlayerStateUpdate;
            localStateUpdateMsg.data.playerStateUpdate = {
                .posX = localPlayer->m_position.x,
                .posY = localPlayer->m_position.y,
                .velX = localPlayer->m_velocity.x,
                .velY = localPlayer->m_velocity.y,
                .state = localPlayer->getState(),
                .playerID = *m_localPlayerID
            };

            std::vector<Message> outMessages{localStateUpdateMsg};
            outMessages.insert(outMessages.end(),
                               m_pendingProjectileSpawnMessages.begin(),
                               m_pendingProjectileSpawnMessages.end());
            m_pendingProjectileSpawnMessages.clear();

            return outMessages;
        }

        void updateSimulation(int deltaTime)
        {
            if (m_localPlayerID)
            {
                std::unique_ptr<Player>& localPlayer = getLocalPlayer();
                localPlayer->update(deltaTime);
            }

            updateProjectiles(deltaTime);
        }

        const std::map<PlayerID, std::unique_ptr<Player>>& getPlayers()
        {
            return m_players;
        }

        const std::map<ProjectileID, Projectile>& getProjectiles()
        {
            return m_projectiles;
        }

        std::vector<ProjectileHit> detectProjectileHits()
        {
            std::vector<ProjectileHit> hits;

            for (const auto& [projectileID, projectile] : m_projectiles)
            {
                SDL_Rect projectileBox = projectile.getBoundingBox();

                for (const auto& [playerID, player] : m_players)
                {
                    if (playerID == projectile.ownerPlayerID)
                    {
                        continue;
                    }

                    SDL_Rect playerBox = player->getBoundingBox();

                    if (hasIntersection(projectileBox, playerBox))
                    {
                        hits.push_back(ProjectileHit{
                            .projectileID = projectileID,
                            .ownerPlayerID = projectile.ownerPlayerID,
                            .hitPlayerID = playerID
                        });
                        break;
                    }
                }
            }

            return hits;
        }

        void removeProjectiles(const std::vector<ProjectileID>& projectileIDs)
        {
            for (ProjectileID projectileID : projectileIDs)
            {
                m_projectiles.erase(projectileID);
            }
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

        void handleProjectileSpawnMessage(ProjectileSpawn projectileSpawn)
        {
            m_projectiles.try_emplace(
                projectileSpawn.projectileID,
                projectileSpawn.projectileID,
                projectileSpawn.ownerPlayerID,
                Vector2D<float>{projectileSpawn.posX, projectileSpawn.posY},
                Vector2D<float>{projectileSpawn.velX, projectileSpawn.velY}
            );
        }

        void spawnLocalProjectile(const Player& player)
        {
            assert(m_localPlayerID);

            ProjectileID projectileID = (*m_localPlayerID * 100000) + m_nextLocalProjectileID++;
            Vector2D<float> velocity = player.m_facingDirection * Projectile::SPEED;

            float spawnX = player.m_position.x + (Player::WIDTH_PX / 2.0f)
                           - (Projectile::SIZE_PX / 2.0f);
            float spawnY = player.m_position.y + (Player::WIDTH_PX / 2.0f)
                           - (Projectile::SIZE_PX / 2.0f);

            ProjectileSpawn projectileSpawn{
                .projectileID = projectileID,
                .ownerPlayerID = *m_localPlayerID,
                .posX = spawnX,
                .posY = spawnY,
                .velX = velocity.x,
                .velY = velocity.y
            };

            handleProjectileSpawnMessage(projectileSpawn);

            Message projectileSpawnMessage{
                .type = MessageType::ProjectileSpawn,
                .data = { .projectileSpawn = projectileSpawn }
            };
            m_pendingProjectileSpawnMessages.push_back(projectileSpawnMessage);
        }

        void updateProjectiles(int deltaTime)
        {
            std::vector<ProjectileID> projectilesToRemove;

            for (auto& [projectileID, projectile] : m_projectiles)
            {
                projectile.update(deltaTime);

                const SDL_Rect box = projectile.getBoundingBox();
                bool outOfBounds = box.x + box.w < 0
                    || box.x > constants::WINDOW_WIDTH
                    || box.y + box.h < 0
                    || box.y > constants::WINDOW_HEIGHT;

                if (outOfBounds || projectile.ageMs >= Projectile::LIFETIME_MS)
                {
                    projectilesToRemove.push_back(projectileID);
                }
            }

            for (ProjectileID projectileID : projectilesToRemove)
            {
                m_projectiles.erase(projectileID);
            }
        }

        bool hasIntersection(const SDL_Rect& a, const SDL_Rect& b)
        {
            return a.x < b.x + b.w
                && a.x + a.w > b.x
                && a.y < b.y + b.h
                && a.y + a.h > b.y;
        }

    private:
        std::optional<PlayerID> m_localPlayerID;
        std::map<PlayerID, std::unique_ptr<Player>> m_players;

        std::map<ProjectileID, Projectile> m_projectiles;
        std::vector<Message> m_pendingProjectileSpawnMessages;
        uint m_nextLocalProjectileID = 0;
};
