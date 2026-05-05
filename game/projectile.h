#pragma once

#include "SDL2/SDL.h"

#include "common/vector_2d.h"

#include "player.h"

using ProjectileID = uint;

class Projectile
{
    public:
        static constexpr int SIZE_PX = 8;
        static constexpr float SPEED = 0.55f; // Pixels per ms
        static constexpr int LIFETIME_MS = 1200;

        Projectile(ProjectileID projectileID,
                   PlayerID ownerPlayerID,
                   Vector2D<float> position,
                   Vector2D<float> velocity)
        : id(projectileID)
        , ownerPlayerID(ownerPlayerID)
        , position(position)
        , velocity(velocity)
        {}

        void update(int deltaTime)
        {
            position += velocity * static_cast<float>(deltaTime);
            ageMs += deltaTime;
        }

        bool isExpired() const
        {
            return ageMs >= LIFETIME_MS;
        }

        SDL_Rect getBoundingBox() const
        {
            return SDL_Rect{
                static_cast<int>(position.x),
                static_cast<int>(position.y),
                SIZE_PX,
                SIZE_PX
            };
        }

        ProjectileID id;
        PlayerID ownerPlayerID;
        Vector2D<float> position;
        Vector2D<float> velocity;
        int ageMs = 0;
};

struct ProjectileHit
{
    ProjectileID projectileID;
    PlayerID ownerPlayerID;
    PlayerID hitPlayerID;
};
