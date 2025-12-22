#ifndef NET_MESSAGES_H_
#define NET_MESSAGES_H_

#include <vector>
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

#endif