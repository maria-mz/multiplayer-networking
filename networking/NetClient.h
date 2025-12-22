#ifndef NET_CLIENT_H_
#define NET_CLIENT_H_

#include <mutex>
#include <thread>

#include <asio.hpp>

#include "../Logging.h"
#include "NetConnection.h"
#include "NetMessages.h"

class NetClient
{
    public:
        ~NetClient();

        bool connectToServer(std::string ip, std::string port, int timeoutMs = 250);
        bool isConnected();
        void disconnect();

        bool send(NetMessage &msg);
        bool recv(NetMessage &msg);
        bool blockingRecv(NetMessage &msg);

    private:
        asio::io_context m_ioContext;
        std::thread m_contextThread;

        std::shared_ptr<NetConnection> m_connection;
};

#endif