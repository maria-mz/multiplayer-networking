#pragma once

#include <unordered_map>
#include <cassert>

#include "SDL2/SDL.h"

#include "common/Constants.h"
#include "common/Vector2D.h"

#include "Event.h"

class InputManager
{
    public:
        enum class Action {
            MoveLeft,
            MoveRight,
            MoveUp,
            MoveDown,
        };

    public:
        InputManager();

        void input(GameEvent inputEvent);
        bool isKeyPressed(Action action);

    private:
        std::unordered_map<Action, bool> m_keyState;
};

class Player;

enum class PlayerState
{
    Idle,
    Run,
};

// Interface for all states
class PlayerStateInterface
{
    public:
        virtual ~PlayerStateInterface() = default;

        virtual void enter(Player &player) = 0;
        virtual void input(Player &player, GameEvent inputEvent) = 0;
        virtual void update(Player &player, int deltaTime) = 0;
        virtual void exit(Player &player) = 0;
};

class PlayerStateIdle : public PlayerStateInterface {
    public:
        void enter(Player &player) override;
        void input(Player &player, GameEvent inputEvent) override;
        void update(Player &player, int deltaTime) override;
        void exit(Player &player) override;
};

class PlayerStateRun : public PlayerStateInterface {
    public:
        void enter(Player &player) override;
        void input(Player &player, GameEvent inputEvent) override;
        void update(Player &player, int deltaTime) override;
        void exit(Player &player) override;
};

struct Transform
{
    float width;
    float height;
    float scale;
};

class Player
{
    public:
        static constexpr int WIDTH_PX = 32;
        static constexpr float SPEED = 0.35; // Pixels per ms

        Player();
        ~Player();

        PlayerState getState() const;
        SDL_Rect getBoundingBox() const;

        void input(GameEvent inputEvent);
        void update(int deltaTime);
        void maybeChangeState(PlayerState state);
        void boundPosition();

        Vector2D<float> m_position;
        Vector2D<float> m_velocity;
        Transform m_transform;

        InputManager m_inputManager;

        bool isLocal = false;

    private:
        PlayerState m_currentState;
        std::unique_ptr<PlayerStateInterface> m_stateObject;
};
