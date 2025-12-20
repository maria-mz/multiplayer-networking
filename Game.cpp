#include "Game.h"


void Game::input(InputEvent event)
{
    m_lastInputEvent = event;
    m_player.input(event);
}

std::vector<GameMessage> Game::update(const int deltaTime,
                                      std::vector<GameMessage>& inMessages)
{
    m_player.update(deltaTime);

    // for (auto& message : inMessages)
    // {
    //     switch (message.type)
    //     {
    //         case GameMessageType::MovementUpdate:
    //             m_player->input(message.data.movementUpdate.inputEvent);
    //             m_player->m_direction = message.data.movementUpdate.direction;
    //     }
    // }

    GameMessage msg;
    msg.type = GameMessageType::MovementUpdate;
    msg.data.movementUpdate = MovementUpdate{
        m_player.m_position.x,
        m_player.m_position.y,
        m_player.m_velocity.x,
        m_player.m_velocity.y,
        m_lastInputEvent,
        m_player.m_direction,
        m_playerID
    };

    return std::vector<GameMessage>{msg};
}
