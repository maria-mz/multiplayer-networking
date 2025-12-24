#pragma once

#include <vector>
#include <iostream>
#include <cassert>
#include <format>

// Forward declare NetConnection, let it access NetMessage body to use as buffer
class NetConnection;

enum class NetMessageType : u_int32_t
{
    Unknown = 0,
    ConnectOk,
    ConnectDenied,
    Disconnect,
    Data
};

inline std::ostream& operator<<(std::ostream& os, NetMessageType type) {
    switch (type) {
        case NetMessageType::ConnectOk:         return os << "ConnectOk";
        case NetMessageType::ConnectDenied:     return os << "ConnectDenied";
        case NetMessageType::Disconnect:        return os << "Disconnect";
        case NetMessageType::Data:              return os << "Data";

        default:                                return os << "Unknown";
    }
}

struct NetMessageHeader
{
    NetMessageType type;
    u_int32_t size;      // Number of bytes in the message body
};

struct ConnectOkBody
{
    u_int32_t assignedClientID;
};

struct NetMessage
{
    public:
        NetMessage() : header{} {}

        NetMessage(NetMessageType messageType) : header{messageType, 0} {}

        template <typename T>
        NetMessage(NetMessageType messageType, const T &data)
        {
            header.type = messageType;

            static_assert(std::is_trivially_copyable<T>::value,
                "Message body must be trivially copyable");

            bodyRaw.resize(sizeof(T));
            std::memcpy(bodyRaw.data(), &data, sizeof(T));
            header.size = sizeof(T);
        }

        template <typename T>
        T body()
        {
            static_assert(std::is_trivially_copyable<T>::value,
                          "Message body must be trivially copyable");

            if (bodyRaw.size() != sizeof(T))
            {
                throw std::runtime_error("Message body size doesn't match type size");
            }

            T data;
            std::memcpy(&data, bodyRaw.data(), sizeof(T));
            return data;
        }

        NetMessageHeader header;

    private:
        std::vector<uint8_t> bodyRaw;

        friend class NetConnection;
};

const size_t MAX_UDP_BYTES_PER_PACKET = 128;

struct UDPDataBuffer
{
    std::array<uint8_t, MAX_UDP_BYTES_PER_PACKET> bytes;
    size_t usedSize = 0;
};

struct UDPPacket
{
    UDPPacket() : dataBuffer{} {}

    template <typename T>
    UDPPacket(const T& data)
    {
        static_assert(std::is_trivially_copyable<T>::value,
            "Data must be trivially copyable");

        assert(sizeof(T) < MAX_UDP_BYTES_PER_PACKET);

        std::memcpy(dataBuffer.bytes.data(), &data, sizeof(T));
        dataBuffer.usedSize = sizeof(T);
    }

    template <typename T>
    T data()
    {
        static_assert(std::is_trivially_copyable<T>::value,
                        "Data must be trivially copyable");

        if (sizeof(T) != dataBuffer.usedSize)
        {
            throw std::runtime_error(
                std::format("Type size ({}) does not match actual packet size ({})",
                            sizeof(T),
                            dataBuffer.usedSize)
            );
        }

        return *reinterpret_cast<const T*>(dataBuffer.bytes.data());
    }

    UDPDataBuffer dataBuffer;
};
