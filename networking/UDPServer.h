#pragma once

#include <asio.hpp>
#include <thread>
#include <atomic>

#include "../common/TSQueue.h"
#include "../common/Logging.h"

#include "NetMessages.h"

class UDPServer
{
    public:
        UDPServer(int serverPort)
        : m_socket(m_ioContext,
                   asio::ip::udp::endpoint(asio::ip::udp::v4(), serverPort))
        {}

        ~UDPServer()
        {
            shutdown();
        }

        void start()
        {
            if (m_running)
            {
                throw std::runtime_error("Server already started");
            }

            startRecv();

            m_contextThread = std::thread(
                [this]()
                {
                    try
                    {
                        LOG_INFO("ASIO context starting");
                        m_ioContext.run();
                        LOG_INFO("ASIO context stopped");
                    }
                    catch (const std::system_error& e)
                    {
                        // Socket already closed
                    }
                }
            );

            m_running = true;
        }

        void sendTo(UDPPacket pkt, asio::ip::udp::endpoint receiver)
        {
            asio::post(
                m_ioContext,
                [pkt, receiver, this]()
                {
                    startSend(pkt, receiver);
                }
            );
        }

        bool recv(UDPPacket &pkt, asio::ip::udp::endpoint& sender)
        {
            bool ok = false;

            if (!m_inPackets.empty())
            {
                auto packetAndSender = m_inPackets.pop();
                pkt = std::get<0>(packetAndSender);
                sender = std::get<1>(packetAndSender);
                ok = true;
            }
            return ok;
        }

        void shutdown()
        {
            if (!m_running)
                return;

            m_running = false;

            m_socket.close();

            m_ioContext.stop();
            if (m_contextThread.joinable())
            {
                LOG_INFO("Joining ASIO context thread");
                m_contextThread.join();
            }
        }

    private:
        void startRecv()
        {
            auto pkt = std::make_shared<UDPPacket>();
            auto sender = std::make_shared<asio::ip::udp::endpoint>();

            m_socket.async_receive_from(
                asio::buffer(pkt->dataBuffer.bytes.data(), pkt->dataBuffer.bytes.size()),
                *sender,
                [this, pkt, sender](const std::error_code& ec, size_t length)
                {
                    if (!m_running)
                    {
                        // Shutting down - do nothing
                        return;
                    }

                    if (!ec)
                    {
                        pkt->dataBuffer.usedSize = length;
                        m_inPackets.push({*pkt, *sender});
                    }
                    else if (ec == asio::error::operation_aborted)
                    {
                        // Expected during shutdown
                        return;
                    }
                    else
                    {
                        LOG_ERROR("Failed to receive packet: %s", ec.message().c_str());
                    }

                    // Only restart if still running
                    if (m_running)
                    {
                        startRecv();
                    }
                }
            );
        }

       void startSend(UDPPacket pkt, asio::ip::udp::endpoint receiver)
        {
            m_socket.async_send_to(
                asio::buffer(pkt.dataBuffer.bytes.data(), pkt.dataBuffer.usedSize),
                receiver,
                [this, pkt, receiver](const std::error_code& ec, size_t length)
                {
                    if (!ec)
                    {
                        return;
                    }
                    if (ec == asio::error::operation_aborted)
                    {
                        // Expected during shutdown
                        return;
                    }
                    else
                    {
                        LOG_ERROR("Failed to send packet to %s:%u: %s",
                            receiver.address().to_string().c_str(),
                            receiver.port(),
                            ec.message().c_str());
                    }
                }
            );
        }

    private:
        asio::io_context m_ioContext;
        std::thread m_contextThread;

        asio::ip::udp::socket m_socket;

        TSQueue<std::pair<UDPPacket, asio::ip::udp::endpoint>> m_inPackets;

        std::atomic<bool> m_running{false};
};
