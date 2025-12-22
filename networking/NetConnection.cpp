#include "NetConnection.h"
#include <iostream>

// std::ostream& operator<<(std::ostream& os, NetMessageType type) {
//     switch (type) {
//         case NetMessageType::ConnectOk:         return os << "ConnectOk";
//         case NetMessageType::ConnectDenied:     return os << "ConnectDenied";
//         case NetMessageType::Disconnect:        return os << "Disconnect";
//         case NetMessageType::Data:              return os << "Data";

//         default:                                return os << "Unknown";
//     }
// }

std::shared_ptr<NetConnection> NetConnection::create(asio::io_context &io_context)
{
    return std::shared_ptr<NetConnection>(new NetConnection(io_context));
}


void NetConnection::readHeader()
{
    m_currentMsgIn = NetMessage();

    asio::async_read(
        m_socket,
        asio::buffer((&m_currentMsgIn.header), sizeof(NetMessageHeader)),
        [this](const std::error_code &ec, size_t length)
        {
            if (!ec)
            {
                if (m_currentMsgIn.header.size > 0)
                {
                    m_currentMsgIn.bodyRaw.resize(m_currentMsgIn.header.size);
                    readBody();
                }
                else
                {
                    m_inMessages.push(m_currentMsgIn);

                    readHeader();
                }
            }
            else
            {
                LOG_DEBUG("Message read failure");
                m_socket.close();
                m_inMessages.push(NetMessage{NetMessageType::Disconnect});
            }
        }
    );
}


void NetConnection::readBody()
{
    asio::async_read(
        m_socket,
        asio::buffer((m_currentMsgIn.bodyRaw.data()), m_currentMsgIn.bodyRaw.size()),
        [this](const std::error_code &ec, size_t length)
        {
            if (!ec)
            {
                // Read a whole message! (header and body)
                m_inMessages.push(m_currentMsgIn);

                readHeader();
            }
            else
            {
                LOG_DEBUG("Message read failure");
                m_socket.close();
                m_inMessages.push(NetMessage{NetMessageType::Disconnect});
            }
        }
    );
}


void NetConnection::writeHeader()
{
    if (!m_outMessages.front(m_currentMsgOut))
    {
        return;
    }

    asio::async_write(
        m_socket,
        asio::buffer(&m_currentMsgOut.header, sizeof(NetMessageHeader)),
        [this](std::error_code ec, std::size_t length)
        {
            if (!ec)
            {
                if (m_currentMsgOut.header.size > 0)
                {
                    writeBody();
                }
                else
                {
                    m_outMessages.pop();

                    if (!m_outMessages.empty())
                    {
                        writeHeader();
                    }
                }
            }
            else
            {
                LOG_DEBUG("Message write failure");
                m_socket.close();
                m_inMessages.push(NetMessage{NetMessageType::Disconnect});
            }
        }
    );
}


void NetConnection::writeBody()
{
    asio::async_write(
        m_socket,
        asio::buffer((m_currentMsgOut.bodyRaw.data()), m_currentMsgOut.bodyRaw.size()),
        [this](std::error_code ec, std::size_t length)
        {
            if (!ec)
            {
                m_outMessages.pop();

                if (!m_outMessages.empty())
                {
                    writeHeader();
                }
            }
            else
            {
                LOG_DEBUG("Message write failure");
                m_socket.close();
                m_inMessages.push(NetMessage{NetMessageType::Disconnect});
            }
        }
    );
}


void NetConnection::startReadLoop()
{
    readHeader();
}


void NetConnection::send(NetMessage &msg)
{
    // Note: This must run in the thread associated with m_ioContext to preserve
    //       the order of writeHeader -> writeBody calls. Otherwise, it's
    //       possible for two writeHeader calls to occur back-to-back,
    //       which can:
    //         - Corrupt a message (the second call overwrites m_currentMsgOut, so
    //           the buffer in the first call may point to freed or stale memory)
    //         - Break the protocol (the receiver expects a certain number of body
    //           bytes but might instead read part of the next header and stall)
    //      Interestingly, this also solves the issue where messages were being
    //      received out of order.
    asio::post(
        m_ioContext,
        [this, msg]()
        {
            if (m_outMessages.empty())
            {
                m_outMessages.push(msg);
                writeHeader();
            }
            else
            {
                m_outMessages.push(msg);
            }
        }
    );
}


bool NetConnection::recv(NetMessage &msg)
{
    bool ok = false;

    if (!m_inMessages.empty())
    {
        msg = m_inMessages.pop();
        ok = true;
    }
    return ok;
}


bool NetConnection::blockingRecv(NetMessage &msg)
{
    // TODO: Add timeout? (In that case, if timeout expires ok = false)
    bool ok = true;

    msg = m_inMessages.pop();

    return ok;
}
