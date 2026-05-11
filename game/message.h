#pragma once

#include "player.h"

#include "networking/tcp/tcp_message.h"
#include "networking/udp/udp_message.h"

enum class MessageType : uint8_t
{
    Unknown = 0,
    AssignLocalPlayerID,
    PlayerStateUpdate,
    PlayerJoined,
    PlayerLeft,
    UDPBindRequest,
    UDPBind,
    UDPBindAck,
    Ping,
    Pong,
    ProjectileSpawn,
    PlayerHealthUpdate,
    PlayerRespawned
};

struct AssignLocalPlayerID
{
    uint playerID;
};

struct PlayerStateUpdate
{
    float posX;
    float posY;
    float velX;
    float velY;
    PlayerState state;
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

struct UDPBindRequest
{
    int token;
};

struct UDPBind
{
    int token;
};

struct UDPBindAck {};

struct Ping
{
    uint seq;
};

struct Pong
{
    uint seq;
};

struct ProjectileSpawn
{
    uint projectileID;
    uint ownerPlayerID;
    float posX;
    float posY;
    float velX;
    float velY;
};

struct PlayerHealthUpdate
{
    uint playerID;
    int health;
};

struct PlayerRespawned
{
    uint playerID;
    float posX;
    float posY;
    int health;
};

struct Message
{
    MessageType type;
    union {
        PlayerStateUpdate playerStateUpdate;
        AssignLocalPlayerID assignLocalPlayerID;
        PlayerJoined playerJoined;
        PlayerLeft playerLeft;
        UDPBindRequest udpBindRequest;
        UDPBind udpBind;
        UDPBindAck udpBindAck;
        Ping ping;
        Pong pong;
        ProjectileSpawn projectileSpawn;
        PlayerHealthUpdate playerHealthUpdate;
        PlayerRespawned playerRespawned;
    } data;
};


inline Message fromTCPMessage(const TCPMessage& tcpMessage)
{
    return tcpMessage.body<Message>();
}

inline Message fromUDPMessage(const UDPMessage& udpMessage)
{
    return udpMessage.data<Message>();
}

inline TCPMessage toTCPMessage(const Message& message)
{
    return TCPMessage{TCPMessageType::Data, message};
}

inline UDPMessage toUDPMessage(const Message& message)
{
    return UDPMessage{message};
}

inline Message makeAssignLocalPlayerIDMessage(PlayerID playerID)
{
    return Message{
        MessageType::AssignLocalPlayerID,
        {
            .assignLocalPlayerID = {
                playerID
            }
        }
    };
}

inline Message makePlayerJoinedMessage(PlayerID playerID)
{
    return Message{
        MessageType::PlayerJoined,
        {
            .playerJoined = {
                playerID
            }
        }
    };
}

inline Message makePlayerLeftMessage(PlayerID playerID)
{
    return Message{
        MessageType::PlayerLeft,
        {
            .playerLeft = {
                playerID
            }
        }
    };
}

inline Message makePlayerStateUpdateMessage(PlayerID playerID, const Player& player)
{
    return Message{
        MessageType::PlayerStateUpdate,
        {
            .playerStateUpdate = {
                .posX = player.m_position.x,
                .posY = player.m_position.y,
                .velX = player.m_velocity.x,
                .velY = player.m_velocity.y,
                .state = player.getState(),
                .playerID = playerID
            }
        }
    };
}

inline Message makePlayerHealthUpdateMessage(PlayerID playerID, int health)
{
    return Message{
        MessageType::PlayerHealthUpdate,
        {
            .playerHealthUpdate = {
                .playerID = playerID,
                .health = health
            }
        }
    };
}
