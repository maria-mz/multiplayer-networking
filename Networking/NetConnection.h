#ifndef NET_CONNECTION_H_
#define NET_CONNECTION_H_

#include <functional>
#include <memory>
#include <chrono>

#include <asio.hpp>

#include "../Logging.h"
#include "../utils/TSQueue.h"
#include "NetMessages.h"


class NetConnection : public std::enable_shared_from_this<NetConnection>
{
    public:
        static std::shared_ptr<NetConnection> create(asio::io_context &io_context);

        void startReadLoop();
        void send(NetMessage &msg);

        bool recv(NetMessage &msg);
        bool blockingRecv(NetMessage &msg);

        asio::ip::tcp::socket m_socket;
        u_int32_t m_id;

    private:
        NetConnection(asio::io_context &io_context)
        : m_ioContext(io_context)
        , m_socket(io_context)
        {}

        void readHeader();
        void readBody();
        void writeHeader();
        void writeBody();

        NetMessage m_currentMsgIn;
        NetMessage m_currentMsgOut;

        TSQueue<NetMessage> m_inMessages;
        TSQueue<NetMessage> m_outMessages;

        asio::io_context &m_ioContext;
};

#endif