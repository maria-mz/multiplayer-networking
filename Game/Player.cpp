#include "Player.h"
#include <iostream>

InputManager::InputManager()
{
    m_keyState = {
        {Action::MoveLeft, false},
        {Action::MoveRight, false},
    };
}

void InputManager::input(GameEvent inputEvent)
{
    switch (inputEvent)
    {
        case GameEvent::MoveLeft_Pressed: m_keyState[Action::MoveLeft] = true; break;
        case GameEvent::MoveRight_Pressed: m_keyState[Action::MoveRight] = true; break;
        case GameEvent::MoveLeft_Released: m_keyState[Action::MoveLeft] = false; break;
        case GameEvent::MoveRight_Released: m_keyState[Action::MoveRight] = false; break;
        default: break;
    }
}

bool InputManager::isKeyPressed(Action action)
{
    return m_keyState[action];
}

void PlayerStateIdle::enter(Player &player)
{

}

void PlayerStateIdle::input(Player &player, GameEvent inputEvent)
{
    switch (inputEvent)
    {
        case GameEvent::MoveLeft_Pressed:
            player.m_velocity.x -= player.SPEED;

            if (
                player.m_inputManager.isKeyPressed(InputManager::Action::MoveLeft) ||
                player.m_inputManager.isKeyPressed(InputManager::Action::MoveRight)
            )
            {
                player.maybeChangeState(PlayerState::Run);
            }

            break;

        case GameEvent::MoveRight_Pressed:
            player.m_velocity.x += player.SPEED;

            if (
                player.m_inputManager.isKeyPressed(InputManager::Action::MoveLeft) ||
                player.m_inputManager.isKeyPressed(InputManager::Action::MoveRight)
            )
            {
                player.maybeChangeState(PlayerState::Run);
            }

            break;

        default:
            break;
    }
}

void PlayerStateIdle::update(Player &player, int deltaTime)
{

}

void PlayerStateIdle::exit(Player &player)
{

}

void PlayerStateRun::enter(Player &player)
{

}

void PlayerStateRun::input(Player &player, GameEvent inputEvent)
{
    switch (inputEvent)
    {
        case GameEvent::MoveLeft_Pressed:
            player.m_velocity.x -= player.SPEED;
            break;

        case GameEvent::MoveRight_Pressed:
            player.m_velocity.x += player.SPEED;
            break;

        case GameEvent::MoveLeft_Released:
            player.m_velocity.x += player.SPEED;

            if (
                !player.m_inputManager.isKeyPressed(InputManager::Action::MoveLeft) &&
                !player.m_inputManager.isKeyPressed(InputManager::Action::MoveRight)
            )
            {
                player.maybeChangeState(PlayerState::Idle);
                player.m_direction = Direction::Left;
            }

            break;

        case GameEvent::MoveRight_Released:
            player.m_velocity.x -= player.SPEED;

            if (
                !player.m_inputManager.isKeyPressed(InputManager::Action::MoveLeft) &&
                !player.m_inputManager.isKeyPressed(InputManager::Action::MoveRight)
            )
            {
                player.maybeChangeState(PlayerState::Idle);
                player.m_direction = Direction::Right;
            }

            break;

        default:
            break;
    }
}

void PlayerStateRun::update(Player &player, int deltaTime)
{
    player.m_position.x += deltaTime * player.m_velocity.x;

    if (player.m_velocity.x > 0)
    {
        player.m_direction = Direction::Right;
    }
    else
    {
        player.m_direction = Direction::Left;
    }
}

void PlayerStateRun::exit(Player &player)
{

}

Player::Player()
    : m_stateObject(std::make_unique<PlayerStateIdle>()),
      m_transform{32, 32, 1},
      m_position{0, 0}
{
    m_stateObject->enter(*this);
}

Player::~Player()
{
    m_stateObject->exit(*this);
}

void Player::input(GameEvent inputEvent)
{
    m_inputManager.input(inputEvent);
    assert(m_stateObject);
    m_stateObject->input(*this, inputEvent);
}

void Player::update(int deltaTime)
{
    m_stateObject->update(*this, deltaTime);
    boundPosition();
}

void Player::render(SDL_Renderer *renderer) const
{
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue
    renderBox(renderer, getBoundingBox());
}

void Player::renderBox(SDL_Renderer *renderer, SDL_Rect box) const
{
    // SDL Seems to draw a single pixel when the area is 0, but ideally
    // if area is 0 don't draw anything
    if (box.w * box.h > 0)
    {
        SDL_RenderDrawRect(renderer, &box);
    }
}

SDL_Rect Player::getBoundingBox() const
{
    SDL_Rect rect{static_cast<int>(m_position.x),
                  static_cast<int>(m_position.y),
                  WIDTH_PX * static_cast<int>(m_transform.scale),
                  WIDTH_PX * static_cast<int>(m_transform.scale)};

    return rect;
}

void Player::maybeChangeState(PlayerState state)
{
    if (m_currentState == state)
    {
        return;
    }

    m_stateObject->exit(*this);

    switch (state)
    {
        case PlayerState::Idle:
            m_stateObject = std::make_unique<PlayerStateIdle>();
            break;
        case PlayerState::Run:
            m_stateObject = std::make_unique<PlayerStateRun>();
            break;
    }

    m_currentState = state;
    m_stateObject->enter(*this);
}

void Player::boundPosition()
{
    SDL_Rect boundingBox = getBoundingBox();

    if (boundingBox.x < 0)
    {
        m_position.x = 0;
    }
    else if (boundingBox.x + boundingBox.w > Constants::WINDOW_WIDTH)
    {
        m_position.x = Constants::WINDOW_WIDTH - boundingBox.w;
    }
}

PlayerState Player::getState() const
{
    return m_currentState;
}

