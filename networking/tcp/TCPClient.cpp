#include "TCPClient.h"


TCPClient::~TCPClient()
{
    disconnect();
}


bool TCPClient::connectToServer(std::string ip, std::string port, int timeoutMs)
{
    LOG_INFO("Connecting to server at %s:%s", ip.c_str(), port.c_str());

    asio::ip::tcp::resolver resolver(m_ioContext);
    asio::steady_timer timer(m_ioContext);
    asio::ip::tcp::socket socket(m_ioContext);

    bool isConnected = false;
    std::error_code ec;

    asio::ip::make_address(ip, ec);
    if (ec)
    {
        LOG_ERROR("Invalid IP format");
        return false;
    }

    asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(ip, port, ec);
    if (ec)
    {
        LOG_ERROR("Resolve failed: %s", ec.message().c_str());
        return false;
    }

    asio::async_connect(socket,
                        endpoints,
                        [&](const std::error_code& connectEc, const asio::ip::tcp::endpoint&)
                        {
                            if (!connectEc)
                            {
                                isConnected = true;
                            }
                            ec = connectEc;
                            timer.cancel();
                        });

    timer.expires_after(std::chrono::milliseconds(timeoutMs));
    timer.async_wait([&](const std::error_code& timerEc)
                    {
                        if (!timerEc && !isConnected)
                        {
                            ec = asio::error::timed_out;
                            socket.cancel();
                        }
                    });

    m_ioContext.run();
    m_ioContext.restart();

    if (!isConnected)
    {
        LOG_ERROR("Connect failed: %s", ec.message().c_str());
        return false;
    }

    m_connection = TCPConnection::create(m_ioContext);
    m_connection->m_socket = std::move(socket);
    m_connection->startReadLoop();

    m_contextThread = std::thread(
        [this]()
        {
            try
            {
                LOG_INFO("ASIO context starting");
                m_ioContext.run();
                LOG_INFO("ASIO context stopped");
            }
            catch (const std::system_error& e)
            {
                // Socket already closed
            }
        }
    );

    return true;
}


bool TCPClient::isConnected()
{
    return (m_connection && m_connection->m_socket.is_open());
}


void TCPClient::disconnect()
{
    if (!m_ioContext.stopped())
    {
        m_ioContext.stop();
    }

    // Using try-catch here because even if we check if the socket is open first,
    // by the time we call close an async read or write in the ASIO context thread
    // may have already called close -> would throw error anyway
    try
    {
        if (m_connection)
        {
            m_connection->m_socket.close();
        }
    }
    catch(const std::system_error& e)
    {
        // Socket already closed
    }

    if (m_contextThread.joinable())
    {
        // This hangs sometimes... whyy??
        LOG_INFO("Joining ASIO context thread");
        m_contextThread.join();
    }
}


bool TCPClient::send(TCPMessage &msg)
{
    bool ok = false;

    if (m_connection)
    {
        m_connection->send(msg);
        ok = true;
    }

    return ok;
}


bool TCPClient::recv(TCPMessage &msg)
{
    bool ok = false;

    if (m_connection)
    {
        ok = m_connection->recv(msg);
    }

    if (msg.header.type == TCPMessageType::Disconnect)
    {
        m_connection->m_socket.close();
    }

    return ok;
}


bool TCPClient::blockingRecv(TCPMessage &msg)
{
    bool ok = false;

    if (m_connection)
    {
        ok = m_connection->blockingRecv(msg);
    }

    if (msg.header.type == TCPMessageType::Disconnect)
    {
        m_connection->m_socket.close();
    }

    return ok;
}
