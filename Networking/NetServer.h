#ifndef NET_SERVER_H_
#define NET_SERVER_H_

#include <asio.hpp>
#include <unordered_map>
#include <set>
#include <mutex>

#include "../Logging.h"
#include "NetConnection.h"
#include "NetMessages.h"


using ClientID = u_int32_t;


class NetServer
{
    public:
        NetServer(int port)
        : m_acceptor(m_ioContext,
                     // binds to 0.0.0.0:<port>
                     asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {}
        ~NetServer();

        void start();
        void shutdown();

        bool send(ClientID clientID, NetMessage &msg);
        void broadcast(NetMessage &msg, std::optional<ClientID> ignoreClientID);
        bool recv(ClientID clientID, NetMessage &msg);
        bool blockingRecv(ClientID clientID, NetMessage &msg);

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

        std::unordered_map<ClientID, std::shared_ptr<NetConnection>> m_clients;
        std::shared_ptr<NetConnection> getClient(ClientID clientID);

        ClientID m_nextNewClientID = 10000;
        ClientID getNewClientID();

        std::function<void(ClientID)> m_onClientConnect = nullptr;
        std::function<void(ClientID)> m_onClientDisconnect = nullptr;
};

#endif