#pragma once

#include <asio.hpp>
#include "udp_message.h"

class UDPTransportInterface
{
    public:
        virtual ~UDPTransportInterface() = default;

        virtual void sendTo(const UDPMessage& msg,
                            const asio::ip::udp::endpoint& receiver) = 0;

        virtual bool tryRecv(UDPMessage& msg,
                             asio::ip::udp::endpoint& sender) = 0;

        virtual bool isOpen() const = 0;
};
