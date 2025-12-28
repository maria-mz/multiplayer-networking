#pragma once

#include <asio.hpp>
#include <unordered_map>
#include <set>
#include <mutex>
#include <queue>

#include "../../common/Logging.h"
#include "TCPConnection.h"
#include "TCPMessage.h"
#include "TCPListenerInterface.h"
#include "TCPConnectionInterface.h"


class TCPListener
: public TCPListenerInterface
, public std::enable_shared_from_this<TCPListener>
{
    public:
        TCPListener(asio::io_context& ioContext, uint16_t port)
        : m_ioContext(ioContext)
        , m_acceptor(ioContext, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
        {}

        ~TCPListener()
        {
            shutdown();
        }

        void start()
        {
            LOG_INFO("Starting accept loop");
            acceptNext();
        }

        void shutdown()
        {
            LOG_INFO("Shutting down listener");
            std::error_code ec;
            m_acceptor.cancel(ec);
            m_acceptor.close(ec);
        }

        std::shared_ptr<TCPConnectionInterface> getNextConnection()
        {
            auto maybeConn = m_newConnections.tryPop();
            return maybeConn ? *maybeConn : nullptr;
        }

        uint16_t getPort() const
        {
            return m_acceptor.local_endpoint().port();
        }

    private:
        void acceptNext()
        {
            auto self = shared_from_this();
            auto socket = std::make_shared<asio::ip::tcp::socket>(m_ioContext);

            m_acceptor.async_accept(*socket,
                [self, socket](const std::error_code& ec) mutable
                {
                    if (!ec)
                    {
                        auto connection = TCPConnection::create(self->m_ioContext, std::move(*socket));
                        connection->start();

                        self->m_newConnections.push(connection);
                        LOG_INFO("Accepted connection");
                    }
                    else
                    {
                        LOG_WARNING("Accept failed: %s", ec.message().c_str());
                    }

                    if (self->m_acceptor.is_open())
                    {
                        self->acceptNext();
                    }
                }
            );
        }

    private:
        asio::io_context& m_ioContext;
        asio::ip::tcp::acceptor m_acceptor;

        TSQueue<std::shared_ptr<TCPConnection>> m_newConnections;
};
