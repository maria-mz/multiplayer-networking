#pragma once

#include <queue>

#include "../../networking/tcp/TCPConnectionInterface.h"
#include "../../networking/tcp/TCPMessage.h"

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
};
