#pragma once

#include <cassert>
#include <format>
#include <cstring>


const size_t MAX_UDP_BYTES_PER_MESSAGE = 128;

struct UDPDataBuffer
{
    std::array<uint8_t, MAX_UDP_BYTES_PER_MESSAGE> bytes;
    size_t usedSize = 0;

    void clear()
    {
        usedSize = 0;
        bytes.fill(0);
    }
};

struct UDPMessage
{
    UDPMessage() : dataBuffer{} {}

    template <typename T>
    UDPMessage(const T& data)
    {
        static_assert(std::is_trivially_copyable<T>::value,
            "Data must be trivially copyable");

        assert(sizeof(T) < MAX_UDP_BYTES_PER_MESSAGE);

        std::memcpy(dataBuffer.bytes.data(), &data, sizeof(T));
        dataBuffer.usedSize = sizeof(T);
    }

    template <typename T>
    T data() const
    {
        static_assert(std::is_trivially_copyable<T>::value,
                        "Data must be trivially copyable");

        if (sizeof(T) != dataBuffer.usedSize)
        {
            throw std::runtime_error(
                std::format("Type size ({}) does not match actual message size ({})",
                            sizeof(T),
                            dataBuffer.usedSize)
            );
        }

        return *reinterpret_cast<const T*>(dataBuffer.bytes.data());
    }

    UDPDataBuffer dataBuffer;
};
