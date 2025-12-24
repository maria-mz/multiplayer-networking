#pragma once

#include <mutex>
#include <thread>

#include <asio.hpp>

#include "../../common/Logging.h"
#include "TCPConnection.h"
#include "TCPMessage.h"

class TCPClient
{
    public:
        ~TCPClient();

        bool connectToServer(std::string ip, std::string port, int timeoutMs = 250);
        bool isConnected();
        void disconnect();

        bool send(TCPMessage &msg);
        bool recv(TCPMessage &msg);
        bool blockingRecv(TCPMessage &msg);

    private:
        asio::io_context m_ioContext;
        std::thread m_contextThread;

        std::shared_ptr<TCPConnection> m_connection;
};
