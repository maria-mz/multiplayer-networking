#pragma once

#include "../../networking/udp/UDPTransportInterface.h"
#include "../../networking/udp/UDPMessage.h"

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
