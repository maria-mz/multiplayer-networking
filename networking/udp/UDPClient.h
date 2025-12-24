#pragma once

#include <asio.hpp>
#include <thread>
#include <atomic>

#include "../../common/TSQueue.h"
#include "../../common/Logging.h"

#include "UDPPacket.h"


class UDPClient
{
    public:
        UDPClient(std::string serverIP, int serverPort)
        : m_socket(m_ioContext)
        {
            std::error_code ec;
            auto serverAddress = asio::ip::make_address(serverIP, ec);
            if (ec)
            {
                throw std::invalid_argument("Invalid IP format");
            }

            m_serverEndpoint = asio::ip::udp::endpoint(serverAddress, serverPort);
        }

        ~UDPClient()
        {
            shutdown();
        }

        void start()
        {
            if (m_running)
            {
                throw std::runtime_error("Client already started");
            }

            m_socket.open(asio::ip::udp::v4());

            // Bind to any available local port
            m_socket.bind(
                asio::ip::udp::endpoint(asio::ip::udp::v4(), 0)
            );

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

        void send(UDPPacket pkt)
        {
            asio::post(
                m_ioContext,
                [pkt, this]()
                {
                    startSend(pkt);
                }
            );
        }

        bool recv(UDPPacket& pkt)
        {
            bool ok = false;

            if (!m_inPackets.empty())
            {
                pkt = m_inPackets.pop();
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

        // Returns the socket's ephemeral port. Intended to only be used for testing.
        int getLocalPort() const
        {
            std::error_code ec;
            auto endpoint = m_socket.local_endpoint(ec);
            if (ec)
            {
                throw std::runtime_error("Failed to get local endpoint: " + ec.message());
            }

            return endpoint.port();
        }

    private:
        void startRecv()
        {
            auto pkt = std::make_shared<UDPPacket>();

            m_socket.async_receive_from(
                asio::buffer(pkt->dataBuffer.bytes.data(), pkt->dataBuffer.bytes.size()),
                m_serverEndpoint,
                [this, pkt](const std::error_code& ec, size_t length)
                {
                    if (!m_running)
                    {
                        // Shutting down, do nothing
                        return;
                    }

                    if (!ec)
                    {
                        pkt->dataBuffer.usedSize = length;
                        m_inPackets.push(*pkt);
                    }
                    else if (ec == asio::error::operation_aborted)
                    {
                        // Expected during shutdown
                        return;
                    }
                    else
                    {
                        LOG_ERROR("Failed to receive packet: %s", ec.message().c_str());
                        shutdown();
                    }

                    // Only restart if still running
                    if (m_running)
                    {
                        startRecv();
                    }
                }
            );
        }

        void startSend(UDPPacket pkt)
        {
            m_socket.async_send_to(
                asio::buffer(pkt.dataBuffer.bytes.data(), pkt.dataBuffer.usedSize),
                m_serverEndpoint,
                [pkt, this](const std::error_code& ec, size_t length)
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
                        LOG_ERROR("Failed to send packet: %s", ec.message().c_str());
                    }
                }
            );
        }

    private:
        asio::io_context m_ioContext;
        std::thread m_contextThread;

        asio::ip::udp::socket m_socket;
        asio::ip::udp::endpoint m_serverEndpoint;

        TSQueue<UDPPacket> m_inPackets;

        std::atomic<bool> m_running{false};
};
