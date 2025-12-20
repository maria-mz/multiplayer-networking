#pragma once

#include "Player.h"
#include "Input.h"


enum class GameMessageType : uint8_t
{
    Unknown = 0,
    JoinGame,
    JoinGameAck,
    MovementUpdate,
};

struct MovementUpdate
{
    float posX;
    float posY;
    float velX;
    float velY;
    InputEvent inputEvent;
    Direction direction;
    int playerID;
};

struct JoinGame
{

};

struct JoinGameAck
{
    int playerID;
};

struct GameMessage
{
    GameMessageType type;
    union {
        MovementUpdate movementUpdate;
        JoinGame joinGame;
        JoinGameAck joinGameAck;
    } data;
};
