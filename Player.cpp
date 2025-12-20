#include "Player.h"
#include <iostream>

InputManager::InputManager()
{
    m_keyState = {
        {Input::MoveLeft, false},
        {Input::MoveRight, false},
    };
}

void InputManager::input(InputEvent inputEvent)
{
    switch (inputEvent)
    {
        case InputEvent::MoveLeft_Pressed: m_keyState[Input::MoveLeft] = true; break;
        case InputEvent::MoveRight_Pressed: m_keyState[Input::MoveRight] = true; break;
        case InputEvent::MoveLeft_Released: m_keyState[Input::MoveLeft] = false; break;
        case InputEvent::MoveRight_Released: m_keyState[Input::MoveRight] = false; break;
        default: break;
    }
}

bool InputManager::isKeyPressed(Input action)
{
    return m_keyState[action];
}

void PlayerStateIdle::enter(Player &player)
{

}

void PlayerStateIdle::input(Player &player, InputEvent inputEvent)
{
    switch (inputEvent)
    {
        case InputEvent::MoveLeft_Pressed:
            player.m_velocity.x -= player.SPEED;

            if (
                player.m_inputManager.isKeyPressed(Input::MoveLeft) ||
                player.m_inputManager.isKeyPressed(Input::MoveRight)
            )
            {
                player.changeState(PlayerState::Run);
            }

            break;

        case InputEvent::MoveRight_Pressed:
            player.m_velocity.x += player.SPEED;

            if (
                player.m_inputManager.isKeyPressed(Input::MoveLeft) ||
                player.m_inputManager.isKeyPressed(Input::MoveRight)
            )
            {
                player.changeState(PlayerState::Run);
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

void PlayerStateRun::input(Player &player, InputEvent inputEvent)
{
    switch (inputEvent)
    {
        case InputEvent::MoveLeft_Pressed:
            player.m_velocity.x -= player.SPEED;
            break;

        case InputEvent::MoveRight_Pressed:
            player.m_velocity.x += player.SPEED;
            break;

        case InputEvent::MoveLeft_Released:
            player.m_velocity.x += player.SPEED;

            if (
                !player.m_inputManager.isKeyPressed(Input::MoveLeft) &&
                !player.m_inputManager.isKeyPressed(Input::MoveRight)
            )
            {
                player.changeState(PlayerState::Idle);
                player.m_direction = Direction::Left;
            }

            break;

        case InputEvent::MoveRight_Released:
            player.m_velocity.x -= player.SPEED;

            if (
                !player.m_inputManager.isKeyPressed(Input::MoveLeft) &&
                !player.m_inputManager.isKeyPressed(Input::MoveRight)
            )
            {
                player.changeState(PlayerState::Idle);
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
    : m_stateObject(new PlayerStateIdle()),
      m_transform{32, 32, 1},
      m_position{0, 0}
{
    m_stateObject->enter(*this);
}

Player::~Player()
{
    m_stateObject->exit(*this);

    delete m_stateObject;
}

void Player::input(InputEvent inputEvent)
{
    m_inputManager.input(inputEvent);
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

void Player::changeState(PlayerState state)
{
    m_stateObject->exit(*this);
    delete m_stateObject;

    switch (state)
    {
        case PlayerState::Idle:
            m_stateObject = new PlayerStateIdle();
            break;
        case PlayerState::Run:
            m_stateObject = new PlayerStateRun();
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

