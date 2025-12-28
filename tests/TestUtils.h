#pragma once

#include <queue>

#include "../networking/tcp/TCPListenerInterface.h"
#include "../networking/tcp/TCPConnectionInterface.h"
#include "../networking/tcp/TCPMessage.h"

#include "../networking/udp/UDPTransportInterface.h"
#include "../networking/udp/UDPMessage.h"


class MockTCPConnection : public TCPConnectionInterface
{
    public:
        std::vector<TCPMessage> sent;
        std::queue<TCPMessage> received;
        bool open = true;

        void send(const TCPMessage& msg) override
        {
            sent.push_back(msg);
        }

        bool tryRecv(TCPMessage& msg) override
        {
            if (received.empty())
                return false;

            msg = received.front();
            received.pop();
            return true;
        }

        bool isOpen() const override { return open; }

        void disconnect() override {};

        u_int32_t getID() const override { return id; };

        void setID(u_int32_t newID) override { id = newID; };

    private:
        u_int32_t id;
};

class MockTCPListener : public TCPListenerInterface
{
    public:
        std::queue<std::shared_ptr<TCPConnectionInterface>> nextConnections;

        std::shared_ptr<TCPConnectionInterface> getNextConnection() override
        {
            if (nextConnections.empty())
                return nullptr;

            auto c = nextConnections.front();
            nextConnections.pop();
            return c;
        }
};

class MockUDPTransport : public UDPTransportInterface
{
    public:
        std::vector<std::pair<UDPMessage, asio::ip::udp::endpoint>> sent;
        std::queue<std::pair<UDPMessage, asio::ip::udp::endpoint>> received;
        bool open = true;

        void sendTo(const UDPMessage& msg,
                    const asio::ip::udp::endpoint& receiver) override
        {
            sent.push_back({msg, receiver});
        }

        bool tryRecv(UDPMessage& msg,
                     asio::ip::udp::endpoint& sender) override
        {
            if (received.empty())
                return false;

            std::tie(msg, sender) = received.front();
            received.pop();
            return true;
        }

        bool isOpen() const override { return open; }
};

// Helper function that waits for a condition or times out
inline bool waitFor(std::function<bool()> condition, int timeoutMs = 200)
{
    auto start = std::chrono::steady_clock::now();
    while (!condition())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        if (std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count() > timeoutMs)
            return false;
    }
    return true;
}
