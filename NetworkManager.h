#ifndef NETWORK_MANAGER_H_
#define NETWORK_MANAGER_H_

#include <thread>
#include <functional>

#include "Logging.h"
#include "Networking/NetClient.h"
#include "Networking/NetServer.h"
#include "Networking/NetMessages.h"
#include "Player.h"

#include "Input.h"

constexpr int SERVER_PORT = 53791;


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


class NetworkManager
{
    public:
        void setIsHost(bool isHost);

        void startServer();
        bool connectToServer(std::string serverIP);

        bool isConnectedToServer();

        void shutdown();

        void sendPlayerMsg(GameMessage &msg);
        bool receiveOpponentMsg(GameMessage &msg);

    private:
        std::unique_ptr<NetClient> m_client;
        std::unique_ptr<NetServer> m_server;

        bool m_isHost;

        // Details relevant for server:
        int m_opponentClientID;
};

#endif