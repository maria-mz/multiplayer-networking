#ifndef PLAYER_H_
#define PLAYER_H_

#include <map>

#include "SDL2/SDL.h"

#include "Constants.h"
#include "Resources.h"
#include "Input.h"
#include "Utils/Vector2D.h"

class InputManager
{
    public:
        InputManager();

        void input(InputEvent inputEvent);
        bool isKeyPressed(Input action);

    private:
        std::unordered_map<Input, bool> m_keyState;
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
        virtual void input(Player &player, InputEvent inputEvent) = 0;
        virtual void update(Player &player, int deltaTime) = 0;
        virtual void exit(Player &player) = 0;
};

class PlayerStateIdle : public PlayerStateInterface {
    public:
        void enter(Player &player) override;
        void input(Player &player, InputEvent inputEvent) override;
        void update(Player &player, int deltaTime) override;
        void exit(Player &player) override;
};

class PlayerStateRun : public PlayerStateInterface {
    public:
        void enter(Player &player) override;
        void input(Player &player, InputEvent inputEvent) override;
        void update(Player &player, int deltaTime) override;
        void exit(Player &player) override;
};

struct Transform
{
    float width;
    float height;
    float scale;
};

enum class Direction
{
    Left,
    Right,
};

class Player
{
    public:
        static constexpr int WIDTH_PX = 32;
        static constexpr float SPEED = 0.35; // Pixels per ms

        Player();
        ~Player();

        void input(InputEvent inputEvent);
        void update(int deltaTime);
        void render(SDL_Renderer *renderer) const;

        PlayerState getState() const;
        void changeState(PlayerState state);

        void updateDirection(Direction direction);
        void boundPosition();

        Vector2D<float> m_position;
        Vector2D<float> m_velocity;
        Transform m_transform;
        Direction m_direction;

        InputManager m_inputManager;

    private:
        SDL_Rect getBoundingBox() const;

        void renderBox(SDL_Renderer *renderer, SDL_Rect box) const;

        PlayerState m_currentState;
        PlayerStateInterface *m_stateObject;
};

#endif