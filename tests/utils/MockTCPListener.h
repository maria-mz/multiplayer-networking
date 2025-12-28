#pragma once

#include <queue>

#include "../../networking/tcp/TCPListenerInterface.h"
#include "../../networking/tcp/TCPConnectionInterface.h"
#include "../../networking/tcp/TCPMessage.h"

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
