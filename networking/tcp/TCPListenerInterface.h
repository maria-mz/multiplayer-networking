#pragma once

#include "TCPMessage.h"
#include "TCPConnectionInterface.h"

class TCPListenerInterface
{
    public:
        virtual ~TCPListenerInterface() = default;

        virtual std::shared_ptr<TCPConnectionInterface> getNextConnection() = 0;
};
