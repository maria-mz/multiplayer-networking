#pragma once

#include <map>
#include <format>
#include <cassert>
#include <vector>

#include "common/logging.h"
#include "common/utils.h"

#include "player.h"
#include "projectile.h"


class GameState
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
        }

        std::unique_ptr<Player>& getPlayer(PlayerID playerID)
        {
            assert(isPlayerInGame(playerID));
            return m_players.at(playerID);
        }

        const std::map<PlayerID, std::unique_ptr<Player>>& getPlayers()
        {
            return m_players;
        }

        void addProjectile(Projectile projectile)
        {
            m_projectiles.emplace(projectile.id, projectile);
        }

        void removeProjectile(ProjectileID projectileID)
        {
            auto it = m_projectiles.find(projectileID);
            if (it != m_projectiles.end()) {
                m_projectiles.erase(it);
            }
        }

        Projectile& getProjectile(ProjectileID projectileID)
        {
            assert(m_projectiles.contains(projectileID));
            return m_projectiles.at(projectileID);
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

        bool isPlayerInGame(PlayerID playerID)
        {
            return m_players.contains(playerID);
        }

    private:
        std::map<PlayerID, std::unique_ptr<Player>> m_players;
        std::map<ProjectileID, Projectile> m_projectiles;
};
