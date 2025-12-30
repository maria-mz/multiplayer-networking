#pragma once

#include <functional>
#include <memory>
#include <chrono>
#include <cassert>

#include <asio.hpp>

#include "../../common/Logging.h"
#include "../../common/TSQueue.h"
#include "UDPMessage.h"
#include "UDPTransportInterface.h"


class UDPTransport
    : public UDPTransportInterface
    , public std::enable_shared_from_this<UDPTransport>
{
    public:
        static std::shared_ptr<UDPTransport> create(asio::io_context &io_context)
        {
            return std::shared_ptr<UDPTransport>(new UDPTransport(io_context));
        }

        UDPTransport(const UDPTransport&) = delete;
        UDPTransport& operator=(const UDPTransport&) = delete;
        UDPTransport(UDPTransport&&) = delete;
        UDPTransport& operator=(UDPTransport&&) = delete;

        ~UDPTransport()
        {
            close();
        }

        void openEphemeral()
        {
            m_socket.open(asio::ip::udp::v4());
            m_socket.bind({asio::ip::udp::v4(), 0});
            startRecv();
            LOG_INFO("UDPTransport opened on port %u", getLocalPort());
        }

        void openBound(u_int16_t port)
        {
            m_socket.open(asio::ip::udp::v4());
            m_socket.bind({asio::ip::udp::v4(), port});
            startRecv();
            LOG_INFO("UDPTransport opened on port %u", getLocalPort());
        }

        void sendTo(const UDPMessage& msg, const asio::ip::udp::endpoint& receiver)
        {
            if (isOpen())
            {
                startSend(msg, receiver);
            }
        }

        bool tryRecv(UDPMessage &msg, asio::ip::udp::endpoint& sender)
        {
            bool ok = false;

            if (!m_inMessages.empty())
            {
                auto messageAndSender = m_inMessages.pop();
                msg = std::get<0>(messageAndSender);
                sender = std::get<1>(messageAndSender);
                ok = true;
            }
            return ok;
        }

        void close()
        {
            if (!m_socket.is_open())
            {
                return;
            }
            auto port = getLocalPort();
            std::error_code ec;
            m_socket.cancel(ec);
            m_socket.close(ec);

            LOG_INFO("UDPTransport closed on port %u", port);
        }

        bool isOpen() const
        {
            return m_socket.is_open();
        }

        u_int32_t getLocalPort() const
        {
            assert(isOpen());

            std::error_code ec;
            auto endpoint = m_socket.local_endpoint(ec);
            if (ec)
            {
                throw std::runtime_error("Failed to get local endpoint: " + ec.message());
            }

            return endpoint.port();
        }

    private:
        UDPTransport(asio::io_context &io_context)
        : m_socket(io_context)
        {}

        void startRecv()
        {
            m_recvMsg.dataBuffer.clear();
            auto self = shared_from_this();

            m_socket.async_receive_from(
                asio::buffer(
                    m_recvMsg.dataBuffer.bytes.data(),
                    m_recvMsg.dataBuffer.bytes.size()
                ),
                m_recvSender,
                [self](const std::error_code& ec, size_t length)
                {
                    if (!ec)
                    {
                        self->m_recvMsg.dataBuffer.usedSize = length;
                        self->m_inMessages.push({self->m_recvMsg, self->m_recvSender});
                    }
                    else if (ec == asio::error::operation_aborted)
                    {
                        return; // normal shutdown
                    }
                    else
                    {
                        LOG_ERROR("Failed to receive message: %s", ec.message().c_str());
                    }

                    if (self->m_socket.is_open())
                    {
                        self->startRecv();
                    }
                }
            );
        }

       void startSend(UDPMessage msg, asio::ip::udp::endpoint receiver)
        {
            m_socket.async_send_to(
                asio::buffer(
                    msg.dataBuffer.bytes.data(),
                    msg.dataBuffer.usedSize
                ),
                receiver,
                [msg, receiver](const std::error_code& ec, size_t length)
                {
                    if (!ec)
                    {
                        return;
                    }
                    else if (ec == asio::error::operation_aborted)
                    {
                        return; // normal shutdown
                    }
                    else
                    {
                        LOG_ERROR("Failed to send message: %s", ec.message().c_str());
                    }
                }
            );
        }

    private:
        asio::ip::udp::socket m_socket;
        TSQueue<std::pair<UDPMessage, asio::ip::udp::endpoint>> m_inMessages;

        UDPMessage m_recvMsg;
        asio::ip::udp::endpoint m_recvSender;
};
