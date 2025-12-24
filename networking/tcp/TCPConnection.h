#pragma once

#include <functional>
#include <memory>
#include <chrono>

#include <asio.hpp>

#include "../../common/Logging.h"
#include "../../common/TSQueue.h"
#include "TCPMessage.h"


class TCPConnection : public std::enable_shared_from_this<TCPConnection>
{
    public:
        static std::shared_ptr<TCPConnection> create(asio::io_context &io_context);

        void startReadLoop();
        void send(TCPMessage &msg);

        bool recv(TCPMessage &msg);
        bool blockingRecv(TCPMessage &msg);

        asio::ip::tcp::socket m_socket;
        u_int32_t m_id;

    private:
        TCPConnection(asio::io_context &io_context)
        : m_ioContext(io_context)
        , m_socket(io_context)
        {}

        void readHeader();
        void readBody();
        void writeHeader();
        void writeBody();

        TCPMessage m_currentMsgIn;
        TCPMessage m_currentMsgOut;

        TSQueue<TCPMessage> m_inMessages;
        TSQueue<TCPMessage> m_outMessages;

        asio::io_context &m_ioContext;
};
