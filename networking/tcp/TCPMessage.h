#pragma once

#include <vector>
#include <iostream>

// Forward declare TCPConnection so it can access TCPMessage body to use as buffer
class TCPConnection;

enum class TCPMessageType : u_int32_t
{
    Unknown = 0,
    Data
};

inline std::ostream& operator<<(std::ostream& os, TCPMessageType type) {
    switch (type) {
        case TCPMessageType::Data:              return os << "Data";
        default:                                return os << "Unknown";
    }
}

struct TCPMessageHeader
{
    TCPMessageType type;
    u_int32_t size;      // Number of bytes in the message body
};

struct TCPMessage
{
    public:
        TCPMessage() : header{} {}

        TCPMessage(TCPMessageType messageType) : header{messageType, 0} {}

        template <typename T>
        TCPMessage(TCPMessageType messageType, const T &data)
        {
            header.type = messageType;

            static_assert(std::is_trivially_copyable<T>::value,
                "Message body must be trivially copyable");

            bodyRaw.resize(sizeof(T));
            std::memcpy(bodyRaw.data(), &data, sizeof(T));
            header.size = sizeof(T);
        }

        template <typename T>
        T body() const
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

        TCPMessageHeader header;

    private:
        std::vector<uint8_t> bodyRaw;

        friend class TCPConnection;
};
