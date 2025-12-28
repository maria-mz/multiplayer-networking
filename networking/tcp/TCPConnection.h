#pragma once

#include <functional>
#include <memory>
#include <chrono>

#include <asio.hpp>

#include "../../common/Logging.h"
#include "../../common/TSQueue.h"
#include "TCPMessage.h"
#include "TCPConnectionInterface.h"


// Note: Uses self = shared_from_this() inside async handlers instead of *this*
//       because it is possible that the TCPConnection object is destroyed before
//       the handler finishes executing (e.g., when the server removes the connection).
//       Confirmed with address sanitizer that this was the cause of seg faults when
//       clients would disconnect. The object should be destroyed after all async
//       handlers finish executing.

class TCPConnection
    : public TCPConnectionInterface
    , public std::enable_shared_from_this<TCPConnection>
{
    public:
        static std::shared_ptr<TCPConnection> create(
            asio::io_context &io_context,
            asio::ip::tcp::socket socket
        )
        {
            return std::shared_ptr<TCPConnection>(
                new TCPConnection(io_context, std::move(socket))
            );
        }

        ~TCPConnection()
        {
            handleDisconnect();
        }

        void start()
        {
            LOG_INFO("Starting read loop (id=%u)", m_id);
            readHeader();
        }

        void send(const TCPMessage &msg)
        {
            if (!isOpen())
            {
                return;
            }

            auto self = shared_from_this();

            // Note: This must run in the thread associated with m_ioContext to preserve
            //       the order of writeHeader -> writeBody calls. Otherwise, it's
            //       possible for two writeHeader calls to occur back-to-back,
            //       which can:
            //         - Corrupt a message (the second call overwrites m_currentMsgOut, so
            //           the buffer in the first call may point to freed or stale memory)
            //         - Break the protocol (the receiver expects a certain number of body
            //           bytes but might instead read part of the next header and stall)
            //      Interestingly, this also solves the issue where messages were being
            //      received out of order.
            asio::post(
                m_ioContext,
                [self, msg]()
                {
                    if (self->m_outMessages.empty())
                    {
                        self->m_outMessages.push(msg);
                        self->writeHeader();
                    }
                    else
                    {
                        self->m_outMessages.push(msg);
                    }
                }
            );
        }

        bool tryRecv(TCPMessage &msg)
        {
            bool ok = false;

            if (!m_inMessages.empty())
            {
                msg = m_inMessages.pop();
                ok = true;
            }
            return ok;
        }

        bool isOpen() const
        {
            return m_socket.is_open();
        }

        void disconnect()
        {
            handleDisconnect();
        }

        u_int32_t getID() const
        {
            return m_id;
        }

        void setID(u_int32_t newID)
        {
            m_id = newID;
        }

    private:
        TCPConnection(asio::io_context &io_context, asio::ip::tcp::socket socket)
        : m_ioContext(io_context)
        , m_socket(std::move(socket))
        {}

        void readHeader()
        {
            m_currentMsgIn = TCPMessage();
            auto self = shared_from_this();

            asio::async_read(
                m_socket,
                asio::buffer((&m_currentMsgIn.header), sizeof(TCPMessageHeader)),
                [self](const std::error_code &ec, size_t length)
                {
                    if (!ec)
                    {
                        if (self->m_currentMsgIn.header.size > 0)
                        {
                            self->m_currentMsgIn.bodyRaw.resize(self->m_currentMsgIn.header.size);
                            self->readBody();
                        }
                        else
                        {
                            self->m_inMessages.push(self->m_currentMsgIn);

                            self->readHeader();
                        }
                    }
                    else
                    {
                        LOG_WARNING("Failed to read header: %s (id=%u)",
                                    ec.message().c_str(),
                                    self->m_id);
                        self->handleDisconnect();
                    }
                }
            );
        }

        void readBody()
        {
            auto self = shared_from_this();

            asio::async_read(
                m_socket,
                asio::buffer((m_currentMsgIn.bodyRaw.data()), m_currentMsgIn.bodyRaw.size()),
                [self](const std::error_code &ec, size_t length)
                {
                    if (!ec)
                    {
                        // Read a whole message! (header and body)
                        self->m_inMessages.push(self->m_currentMsgIn);

                        self->readHeader();
                    }
                    else
                    {
                        LOG_WARNING("Failed to read body: %s (id=%u)",
                                    ec.message().c_str(),
                                    self->m_id);
                        self->handleDisconnect();
                    }
                }
            );
        }

        void writeHeader()
        {
            if (!m_outMessages.front(m_currentMsgOut))
            {
                return;
            }

            auto self = shared_from_this();

            asio::async_write(
                m_socket,
                asio::buffer(&m_currentMsgOut.header, sizeof(TCPMessageHeader)),
                [self](std::error_code ec, std::size_t length)
                {
                    if (!ec)
                    {
                        if (self->m_currentMsgOut.header.size > 0)
                        {
                            self->writeBody();
                        }
                        else
                        {
                            self->m_outMessages.pop();

                            if (!self->m_outMessages.empty())
                            {
                                self->writeHeader();
                            }
                        }
                    }
                    else
                    {
                        LOG_WARNING("Failed to write header: %s (id=%u)",
                                    ec.message().c_str(),
                                    self->m_id);
                        self->handleDisconnect();
                    }
                }
            );
        }

        void writeBody()
        {
            auto self = shared_from_this();

            asio::async_write(
                m_socket,
                asio::buffer((m_currentMsgOut.bodyRaw.data()), m_currentMsgOut.bodyRaw.size()),
                [self](std::error_code ec, std::size_t length)
                {
                    if (!ec)
                    {
                        self->m_outMessages.pop();

                        if (!self->m_outMessages.empty())
                        {
                            self->writeHeader();
                        }
                    }
                    else
                    {
                        LOG_WARNING("Failed to write body: %s (id=%u)",
                                    ec.message().c_str(),
                                    self->m_id);
                        self->handleDisconnect();
                    }
                }
            );
        }

        void handleDisconnect()
        {
            if (m_disconnected.exchange(true))
                return; // already handled

            LOG_INFO("Disconnecting (id=%u)", m_id);

            std::error_code ec;
            m_socket.cancel(ec);
            m_socket.close(ec);
        }

    private:
        TCPMessage m_currentMsgIn;
        TCPMessage m_currentMsgOut;

        TSQueue<TCPMessage> m_inMessages;
        TSQueue<TCPMessage> m_outMessages;

        asio::io_context& m_ioContext;
        asio::ip::tcp::socket m_socket;

        std::atomic<bool> m_disconnected{false};

        u_int32_t m_id;
};
