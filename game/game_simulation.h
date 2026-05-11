#pragma once

#include <map>
#include <format>
#include <cassert>
#include <optional>
#include <vector>

#include "common/logging.h"
#include "common/constants.h"

#include "game_state.h"
#include "player.h"
#include "projectile.h"
#include "message.h"


class GameSimulation
{
    public:
        struct Config
        {
            uint projectileDamage = 25;
            bool registerProjectileHits = false;
        };

    public:
        GameSimulation(Config config) : m_config(config) {}

        void addPlayer(PlayerID playerID)
        {
            m_gameState.addPlayer(playerID);
        }

        void removePlayer(PlayerID playerID)
        {
            m_gameState.removePlayer(playerID);
        }

        void applyPlayerInput(PlayerID playerID, GameEvent event)
        {
            if (event == GameEvent::Shoot_Pressed)
            {
                handleShootInput(playerID);
            }
            else if (event != GameEvent::None)
            {
                auto& player = m_gameState.getPlayer(playerID);
                player->input(event);
            }
        }

        void setPlayerIsLocal(PlayerID playerID)
        {
            auto& player = m_gameState.getPlayer(playerID);
            player->isLocal = true;
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
                case MessageType::PlayerHealthUpdate:
                    handlePlayerHealthUpdateMessage(message.data.playerHealthUpdate);
                    break;
                case MessageType::PlayerRespawned:
                    handlePlayerRespawnedMessage(message.data.playerRespawned);
                    break;
                default:
                    break;
            }
        }

        void updateSimulation(int deltaTime)
        {
            updateProjectiles(deltaTime);
            updatePlayers(deltaTime);
            handleProjectileHits();
        }

        void updateSimulation(int deltaTime, PlayerID playerToUpdate) {
            updateProjectiles(deltaTime);
            updatePlayer(deltaTime, playerToUpdate);
            handleProjectileHits();
        }

        std::vector<Message> collectOutgoingMessages()
        {
            std::vector<Message> messages;
            messages.swap(m_outgoingMessages);
            return messages;
        }

        const std::map<PlayerID, std::unique_ptr<Player>>& getPlayers()
        {
            return m_gameState.getPlayers();
        }

        const std::unique_ptr<Player>& getPlayer(PlayerID playerID)
        {
            return m_gameState.getPlayer(playerID);
        }

        const std::map<ProjectileID, Projectile>& getProjectiles()
        {
            return m_gameState.getProjectiles();
        }

        bool isPlayerInGame(PlayerID playerID)
        {
            return m_gameState.isPlayerInGame(playerID);
        }

    private:
        void updatePlayers(int deltaTime)
        {
            for (const auto& [id, player] : m_gameState.getPlayers())
            {
                player->update(deltaTime);
            }
        }

        void updatePlayer(int deltaTime, PlayerID playerID)
        {
            auto& player = m_gameState.getPlayer(playerID);
            player->update(deltaTime);
        }

        void handleShootInput(PlayerID playerID)
        {
            Projectile spawnedProjectile = createProjectile(playerID);

            m_gameState.addProjectile(spawnedProjectile);

            Message projectileSpawnMessage = Message{
                .type = MessageType::ProjectileSpawn,
                .data = {
                    .projectileSpawn = {
                        .projectileID = spawnedProjectile.id,
                        .ownerPlayerID = spawnedProjectile.ownerPlayerID,
                        .posX = spawnedProjectile.position.x,
                        .posY = spawnedProjectile.position.y,
                        .velX = spawnedProjectile.velocity.x,
                        .velY = spawnedProjectile.velocity.y
                    }
                }
            };
            m_outgoingMessages.push_back(projectileSpawnMessage);
        }

        Projectile createProjectile(PlayerID ownerPlayerID)
        {
            auto& player = m_gameState.getPlayer(ownerPlayerID);

            ProjectileID projectileID = generateProjectileID(ownerPlayerID);

            Vector2D<float> velocity = player->m_facingDirection * Projectile::SPEED;
            Vector2D<float> spawn{
                player->m_position.x + (Player::WIDTH_PX / 2.0f) - (Projectile::SIZE_PX / 2.0f),
                player->m_position.y + (Player::WIDTH_PX / 2.0f) - (Projectile::SIZE_PX / 2.0f)
            };

            return Projectile(projectileID, ownerPlayerID, spawn, velocity);
        }

        ProjectileID generateProjectileID(PlayerID ownerPlayerID) {
            return (ownerPlayerID * 100000) + m_nextProjectileID++;
        }

        void handleProjectileHits()
        {
            auto hits = m_gameState.detectProjectileHits();
            std::vector<ProjectileID> hitProjectileIDs;

            for (const auto& hit : hits)
            {
                if (m_config.registerProjectileHits)
                {
                    registerPlayerHit(hit);
                }
                hitProjectileIDs.push_back(hit.projectileID);
            }

            for (ProjectileID projectileID : hitProjectileIDs)
            {
                m_gameState.removeProjectile(projectileID);
            }
        }

        void registerPlayerHit(ProjectileHit hit) {
            auto& player = m_gameState.getPlayer(hit.hitPlayerID);
            player->applyDamage(m_config.projectileDamage);

            int newHealth = player->getHealth();
            if (newHealth <= 0)
            {
                PlayerRespawned playerRespawnedMsg = respawnPlayer(hit.hitPlayerID);
                m_outgoingMessages.push_back(Message{
                    .type = MessageType::PlayerRespawned,
                    .data = { .playerRespawned = playerRespawnedMsg }
                });
            }
            else
            {
                m_outgoingMessages.push_back(Message{
                    .type = MessageType::PlayerHealthUpdate,
                    .data = { .playerHealthUpdate = PlayerHealthUpdate{
                        .playerID = hit.hitPlayerID,
                        .health = newHealth
                    }}
                });
            }
        }

        PlayerRespawned respawnPlayer(PlayerID playerID)
        {
            auto& player = m_gameState.getPlayer(playerID);
            Vector2D<float> spawnPosition = getPlayerSpawnPosition();

            player->m_position = spawnPosition;
            player->m_velocity = {0.0f, 0.0f};
            player->setHealth(Player::MAX_HEALTH);

            return PlayerRespawned{
                .playerID = playerID,
                .posX = spawnPosition.x,
                .posY = spawnPosition.y,
                .health = player->getHealth()
            };
        }

        Vector2D<float> getPlayerSpawnPosition()
        {
            return {
                (constants::WINDOW_WIDTH / 2.0f) - (Player::WIDTH_PX / 2.0f),
                (constants::WINDOW_HEIGHT / 2.0f) - (Player::WIDTH_PX / 2.0f)
            };
        }

        void updatePlayerState(PlayerID playerID, const PlayerStateUpdate& stateUpdate)
        {
            auto& player = m_gameState.getPlayer(playerID);

            player->maybeChangeState(stateUpdate.state);
            player->m_position.x = stateUpdate.posX;
            player->m_position.y = stateUpdate.posY;
            player->m_velocity.x = stateUpdate.velX;
            player->m_velocity.y = stateUpdate.velY;

            LOG_DEBUG("Updated player %u state: pos=(%f, %f), vel=(%f, %f)",
                    playerID,
                    stateUpdate.posX, stateUpdate.posY,
                    stateUpdate.velX, stateUpdate.velY);
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
            if (!isPlayerInGame(playerStateUpdate.playerID))
            {
                // Assuming no malicious actors, we just missed the player join message,
                // so adding the player now
                addPlayer(playerStateUpdate.playerID);
            }

            updatePlayerState(playerStateUpdate.playerID, playerStateUpdate);
        }

        void handleProjectileSpawnMessage(ProjectileSpawn projectileSpawn)
        {
            m_gameState.addProjectile(
                Projectile(projectileSpawn.projectileID,
                           projectileSpawn.ownerPlayerID,
                           Vector2D<float>{projectileSpawn.posX, projectileSpawn.posY},
                           Vector2D<float>{projectileSpawn.velX, projectileSpawn.velY})
            );
        }

        void handlePlayerHealthUpdateMessage(PlayerHealthUpdate playerHealthUpdate)
        {
            if (!isPlayerInGame(playerHealthUpdate.playerID))
            {
                LOG_WARNING("Received health update for missing player %u",
                            playerHealthUpdate.playerID);
                return;
            }
            auto& player = m_gameState.getPlayer(playerHealthUpdate.playerID);
            player->setHealth(playerHealthUpdate.health);
        }

        void handlePlayerRespawnedMessage(PlayerRespawned playerRespawned)
        {
            if (!isPlayerInGame(playerRespawned.playerID))
            {
                LOG_WARNING("Received respawn for missing player %u",
                            playerRespawned.playerID);
                return;
            }

            auto& player = m_gameState.getPlayer(playerRespawned.playerID);
            player->m_position = {playerRespawned.posX, playerRespawned.posY};
            player->m_velocity = {0.0f, 0.0f};
            player->setHealth(playerRespawned.health);
        }

        void updateProjectiles(int deltaTime)
        {
            std::vector<ProjectileID> projectilesToRemove;

            for (auto& [projectileID, _] : m_gameState.getProjectiles())
            {
                Projectile& projectile = m_gameState.getProjectile(projectileID);
                projectile.update(deltaTime);

                if (isOutOfBounds(projectile) || projectile.isExpired())
                {
                    projectilesToRemove.push_back(projectileID);
                }
            }

            for (ProjectileID projectileID : projectilesToRemove)
            {
                m_gameState.removeProjectile(projectileID);
            }
        }

        bool isOutOfBounds(const Projectile& projectile)
        {
            const SDL_Rect box = projectile.getBoundingBox();
            return box.x + box.w < 0
                || box.x > constants::WINDOW_WIDTH
                || box.y + box.h < 0
                || box.y > constants::WINDOW_HEIGHT;
        }

    private:
        GameState m_gameState;
        uint m_nextProjectileID = 0;

        std::vector<Message> m_outgoingMessages;

        Config m_config;
};
