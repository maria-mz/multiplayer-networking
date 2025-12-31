#include "player.h"
#include <iostream>

InputManager::InputManager()
{
    m_keyState = {
        {Action::MoveLeft, false},
        {Action::MoveRight, false},
        {Action::MoveUp, false},
        {Action::MoveDown, false},
    };
}

void InputManager::input(GameEvent inputEvent)
{
    switch (inputEvent)
    {
        case GameEvent::MoveLeft_Pressed: m_keyState[Action::MoveLeft] = true; break;
        case GameEvent::MoveLeft_Released: m_keyState[Action::MoveLeft] = false; break;
        case GameEvent::MoveRight_Pressed: m_keyState[Action::MoveRight] = true; break;
        case GameEvent::MoveRight_Released: m_keyState[Action::MoveRight] = false; break;
        case GameEvent::MoveUp_Pressed: m_keyState[Action::MoveUp] = true; break;
        case GameEvent::MoveUp_Released: m_keyState[Action::MoveUp] = false; break;
        case GameEvent::MoveDown_Pressed: m_keyState[Action::MoveDown] = true; break;
        case GameEvent::MoveDown_Released: m_keyState[Action::MoveDown] = false; break;
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
    if (
        player.m_inputManager.isKeyPressed(InputManager::Action::MoveLeft) ||
        player.m_inputManager.isKeyPressed(InputManager::Action::MoveRight) ||
        player.m_inputManager.isKeyPressed(InputManager::Action::MoveUp) ||
        player.m_inputManager.isKeyPressed(InputManager::Action::MoveDown)
    )
    {
        player.maybeChangeState(PlayerState::Run);
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
    if (
        !player.m_inputManager.isKeyPressed(InputManager::Action::MoveLeft) &&
        !player.m_inputManager.isKeyPressed(InputManager::Action::MoveRight) &&
        !player.m_inputManager.isKeyPressed(InputManager::Action::MoveUp) &&
        !player.m_inputManager.isKeyPressed(InputManager::Action::MoveDown)
    )
    {
        player.maybeChangeState(PlayerState::Idle);
    }
}

void PlayerStateRun::update(Player &player, int deltaTime)
{
    Vector2D dir{0.0f, 0.0f};

    bool isMovingLeft = player.m_inputManager.isKeyPressed(InputManager::Action::MoveLeft);
    bool isMovingRight = player.m_inputManager.isKeyPressed(InputManager::Action::MoveRight);
    bool isMovingUp = player.m_inputManager.isKeyPressed(InputManager::Action::MoveUp);
    bool isMovingDown = player.m_inputManager.isKeyPressed(InputManager::Action::MoveDown);

    if (isMovingLeft)
        dir.x -= 1.0f;
    if (isMovingRight)
        dir.x += 1.0f;
    if (isMovingUp)
        dir.y -= 1.0f;
    if (isMovingDown)
        dir.y += 1.0f;

    if (dir.x != 0.0f || dir.y != 0.0f)
    {
        // Normalize direction to prevent faster diagonal movement
        float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
        dir.x /= len;
        dir.y /= len;
    }

    player.m_velocity.x = dir.x * player.SPEED;
    player.m_velocity.y = dir.y * player.SPEED;
    player.m_position.x += player.m_velocity.x * deltaTime;
    player.m_position.y += player.m_velocity.y * deltaTime;
}

void PlayerStateRun::exit(Player &player)
{
    player.m_velocity.x = 0;
    player.m_velocity.y = 0;
}

Player::Player()
    : m_stateObject(std::make_unique<PlayerStateIdle>()),
      m_transform{WIDTH_PX, WIDTH_PX, 1},
      m_position{(constants::WINDOW_WIDTH / 2) - (WIDTH_PX / 2),
                 (constants::WINDOW_HEIGHT / 2) - (WIDTH_PX / 2)}
{
    m_stateObject->enter(*this);
}

Player::~Player()
{
    m_stateObject->exit(*this);
}

PlayerState Player::getState() const
{
    return m_currentState;
}

SDL_Rect Player::getBoundingBox() const
{
    SDL_Rect rect{static_cast<int>(m_position.x),
                  static_cast<int>(m_position.y),
                  WIDTH_PX * static_cast<int>(m_transform.scale),
                  WIDTH_PX * static_cast<int>(m_transform.scale)};

    return rect;
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

    // Bound x
    if (boundingBox.x < 0)
    {
        m_position.x = 0;
    }
    else if (boundingBox.x + boundingBox.w > constants::WINDOW_WIDTH)
    {
        m_position.x = constants::WINDOW_WIDTH - boundingBox.w;
    }

    // Bound y
    if (boundingBox.y < 0)
    {
        m_position.y = 0;
    }
    else if (boundingBox.y + boundingBox.h > constants::WINDOW_HEIGHT)
    {
        m_position.y = constants::WINDOW_HEIGHT - boundingBox.h;
    }
}
