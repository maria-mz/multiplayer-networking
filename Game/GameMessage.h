#pragma once

#include "../Player.h"
#include "../Input.h"


enum class GameMessageType : uint8_t
{
    Unknown = 0,
    Welcome,
    PlayerStateUpdate,
    PlayerJoined,
    PlayerLeft
};

struct Welcome
{
    uint playerID;
};

struct PlayerStateUpdate
{
    float posX;
    float posY;
    float velX;
    float velY;
    InputEvent inputEvent;
    Direction direction;
    uint playerID;
};

struct PlayerJoined
{
    uint playerID;
};

struct PlayerLeft
{
    uint playerID;
};

struct GameMessage
{
    GameMessageType type;
    union {
        PlayerStateUpdate playerStateUpdate;
        Welcome welcome;
        PlayerJoined playerJoined;
        PlayerLeft playerLeft;
    } data;
};
