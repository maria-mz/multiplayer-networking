#pragma once

#include "tcp_message.h"
#include "tcp_connection_interface.h"

class TCPListenerInterface
{
    public:
        virtual ~TCPListenerInterface() = default;

        virtual std::shared_ptr<TCPConnectionInterface> getNextConnection() = 0;
};
