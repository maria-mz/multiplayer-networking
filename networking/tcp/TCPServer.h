#pragma once

#include <asio.hpp>
#include <unordered_map>
#include <set>
#include <mutex>

#include "../../common/Logging.h"
#include "TCPConnection.h"
#include "TCPMessage.h"


using ClientID = u_int32_t;


class TCPServer
{
    public:
        TCPServer(int port)
        : m_acceptor(m_ioContext,
                     // binds to 0.0.0.0:<port>
                     asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {}
        ~TCPServer();

        void start();
        void shutdown();

        bool send(ClientID clientID, TCPMessage &msg);
        void broadcast(TCPMessage &msg, std::optional<ClientID> ignoreClientID);
        bool recv(ClientID clientID, TCPMessage &msg);
        bool blockingRecv(ClientID clientID, TCPMessage &msg);

        bool isClientConnected(ClientID clientID);
        void disconnectClient(ClientID clientID);
        void removeClient(ClientID clientID);

        void setOnClientConnect(std::function<void(ClientID)> callback) { m_onClientConnect = std::move(callback); }
        void setOnClientDisconnect(std::function<void(ClientID)> callback) { m_onClientDisconnect = std::move(callback); }

        std::vector<ClientID> getClientIDs();

    private:
        void accept();

        asio::io_context m_ioContext;
        std::thread m_contextThread;

        asio::ip::tcp::acceptor m_acceptor;

        std::unordered_map<ClientID, std::shared_ptr<TCPConnection>> m_clients;
        std::shared_ptr<TCPConnection> getClient(ClientID clientID);

        ClientID m_nextNewClientID = 10000;
        ClientID getNewClientID();

        std::function<void(ClientID)> m_onClientConnect = nullptr;
        std::function<void(ClientID)> m_onClientDisconnect = nullptr;
};
