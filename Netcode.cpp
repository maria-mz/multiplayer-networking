#include "Netcode.h"

void Netcode::updateNetState(MovementUpdate &movementUpdate)
{
    m_netPlayerData.position.x = movementUpdate.posX;
    m_netPlayerData.position.y = movementUpdate.posY;
    m_netPlayerData.velocity.x = movementUpdate.velX;
    m_netPlayerData.velocity.y = movementUpdate.velY;
}

void Netcode::setNetPlayerData(NetPlayerData data)
{
    m_netPlayerData.position.x = data.position.x;
    m_netPlayerData.position.y = data.position.y;
    m_netPlayerData.velocity.x = data.velocity.x;
    m_netPlayerData.velocity.y = data.velocity.y;
}

bool Netcode::isPlayerDesynced(Player &player)
{
    float dx = player.m_position.x - m_netPlayerData.position.x;
    float dy = player.m_position.y - m_netPlayerData.position.y;

    return (abs(dx) >= 1 || abs(dy) >= 1);
}

void Netcode::syncPlayerWithNetState(Player &player)
{
    if (!isPlayerDesynced(player))
    {
        return;
    }

    float dx = player.m_position.x - m_netPlayerData.position.x;
    float dy = player.m_position.y - m_netPlayerData.position.y;
    float dvx = player.m_velocity.x - m_netPlayerData.velocity.x;
    float dvy = player.m_velocity.y - m_netPlayerData.velocity.y;

    player.m_velocity.x = m_netPlayerData.velocity.x;
    player.m_velocity.y = m_netPlayerData.velocity.y;

    const float lerpFactor = 0.3f;

    float lerpedX = std::lerp(player.m_position.x, m_netPlayerData.position.x, lerpFactor);
    float lerpedY = std::lerp(player.m_position.y, m_netPlayerData.position.y, lerpFactor);

    LOG_DEBUG(
        "Correcting opponent "
        "Pos: predicted (%.6f, %.6f) | net (%.6f, %.6f) | "
        "Vel: predicted (%.6f, %.6f) | net (%.6f, %.6f) | "
        "ΔPos: (%.6f, %.6f) | ΔVel: (%.6f, %.6f) | "
        "New Pos (lerped): (%.6f, %.6f)",
        player.m_position.x, player.m_position.y,
        m_netPlayerData.position.x, m_netPlayerData.position.y,
        player.m_velocity.x, player.m_velocity.y,
        m_netPlayerData.velocity.x, m_netPlayerData.velocity.y,
        dx, dy, dvx, dvy,
        lerpedX,
        lerpedY
    );

    player.m_position.x = lerpedX;
    player.m_position.y = lerpedY;
}
