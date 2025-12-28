#pragma once

#include "TCPMessage.h"

class TCPConnectionInterface
{
    public:
        virtual ~TCPConnectionInterface() = default;

        virtual void send(const TCPMessage& msg) = 0;
        virtual bool tryRecv(TCPMessage& msg) = 0;
        virtual bool isOpen() const = 0;
        virtual void disconnect() = 0;
        virtual void setID(u_int32_t newID) = 0;
        virtual u_int32_t getID() const = 0;
};
