#pragma once

#include "Player.h"

#include "../networking/tcp/TCPMessage.h"
#include "../networking/udp/UDPMessage.h"

enum class MessageType : uint8_t
{
    Unknown = 0,
    AssignLocalPlayerID,
    PlayerStateUpdate,
    PlayerJoined,
    PlayerLeft,
    UDPBindRequest,
    UDPBind,
    UDPBindAck
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
