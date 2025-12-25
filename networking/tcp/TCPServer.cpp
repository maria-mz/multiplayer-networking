#include "TCPServer.h"


TCPServer::~TCPServer()
{
    shutdown();
}


void TCPServer::accept()
{
    std::shared_ptr<TCPConnection> newClient = TCPConnection::create(m_ioContext);

    m_acceptor.async_accept(
        newClient->m_socket,
        [this, newClient](const std::error_code &ec)
        {
            if (!ec)
            {
                ClientID clientID = getNewClientID();
                newClient->m_id = clientID;

                LOG_INFO("Accepting new client with ID %d", clientID);
                m_clients[clientID] = newClient;

                newClient->startReadLoop();

                TCPMessage msg{TCPMessageType::ConnectOk, ConnectOkBody{clientID}};
                newClient->send(msg);

                if (m_onClientConnect)
                {
                    m_onClientConnect(clientID);
                }
            }

            accept();
        }
    );
}


ClientID TCPServer::getNewClientID()
{
    return m_nextNewClientID++;
}


void TCPServer::disconnectClient(ClientID clientID)
{
    std::shared_ptr<TCPConnection> client = getClient(clientID);

    if (client)
    {
        // Using try-catch here because even if we check if the socket is open first,
        // by the time we call close an async read or write in the ASIO context thread
        // may have already called close -> would throw error anyway
        try
        {
            client->m_socket.close();

            LOG_INFO("Client with ID %d disconnected", clientID);

            if (m_onClientDisconnect)
            {
                m_onClientDisconnect(client->m_id);
            }
        }
        catch(const std::system_error& e)
        {
            // Socket already closed
        }
    }
}


void TCPServer::removeClient(ClientID clientID)
{
    disconnectClient(clientID);
    m_clients.erase(clientID);
}


void TCPServer::start()
{
    accept();
    m_contextThread = std::thread(
        [this]()
        {
            try
            {
                LOG_INFO("ASIO context starting");
                m_ioContext.run();
                LOG_INFO("ASIO context stopped");
            }
            catch (const std::exception& e) {
                LOG_ERROR("ASIO context exception: %s", e.what());
            }
        }
    );
    LOG_INFO("Started TCP server");
}


void TCPServer::shutdown()
{
    m_ioContext.stop();

    std::error_code ec;
    m_acceptor.cancel(ec);
    m_acceptor.close(ec);

    std::vector<ClientID> clientIDs;

    for (auto &pair : m_clients)
    {
        disconnectClient(pair.first);
    }

    m_clients.clear();

    if (m_contextThread.joinable())
    {
        LOG_INFO("Joining ASIO context thread");
        m_contextThread.join();
    }

    LOG_INFO("Shutdown TCP server");
}


bool TCPServer::send(ClientID clientID, TCPMessage &msg)
{
    bool ok = false;

    std::shared_ptr<TCPConnection> client = getClient(clientID);

    if (client)
    {
        client->send(msg);
        ok = true;
    }

    return ok;
}


void TCPServer::broadcast(TCPMessage &msg, std::optional<ClientID> ignoreClientID)
{
    for (auto &pair : m_clients)
    {
        if (!ignoreClientID || pair.first != *ignoreClientID)
        {
            pair.second->send(msg);
        }
    }
}


bool TCPServer::recv(ClientID clientID, TCPMessage &msg)
{
    bool ok = false;

    std::shared_ptr<TCPConnection> client = getClient(clientID);

    if (client)
    {
        ok = client->recv(msg);
    }

    if (msg.header.type == TCPMessageType::Disconnect)
    {
        removeClient(clientID);
    }

    return ok;
}


bool TCPServer::blockingRecv(ClientID clientID, TCPMessage &msg)
{
    bool ok = false;

    std::shared_ptr<TCPConnection> client = getClient(clientID);

    if (client)
    {
        ok = client->blockingRecv(msg);
    }

    if (msg.header.type == TCPMessageType::Disconnect)
    {
        removeClient(clientID);
    }

    return ok;
}


std::shared_ptr<TCPConnection> TCPServer::getClient(ClientID clientID)
{
    auto it = m_clients.find(clientID);

    if (it != m_clients.end())
    {
        return it->second;
    }

    return nullptr;
}


std::vector<ClientID> TCPServer::getClientIDs()
{
    std::vector<ClientID> ids;
    ids.reserve(m_clients.size());

    for (const auto& pair : m_clients) {
        ids.push_back(pair.first);
    }

    return ids;
}


bool TCPServer::isClientConnected(ClientID clientID)
{
    std::shared_ptr<TCPConnection> client = getClient(clientID);

    if (client && client->m_socket.is_open())
    {
        return true;
    }
    else
    {
        return false;
    }
}
