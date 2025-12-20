#include "NetworkManager.h"

void NetworkManager::setIsHost(bool isHost)
{
    m_isHost = isHost;
}

void NetworkManager::startServer()
{
    if (!m_isHost)
    {
        throw std::logic_error("Only a host can start the server");
    }

    m_server = std::unique_ptr<NetServer>(new NetServer(SERVER_PORT));
    m_server->start();

    m_server->setOnClientConnect([this](ClientID clientID) {
        m_opponentClientID = clientID;
    });
}

bool NetworkManager::connectToServer(std::string serverIP)
{
    if (m_isHost)
    {
        throw std::logic_error("A host cannot connect to the server (it is the server)");
    }

    m_client = std::unique_ptr<NetClient>(new NetClient());

    bool isConnected = m_client->connectToServer(serverIP, std::to_string(SERVER_PORT));

    return isConnected;
}

bool NetworkManager::isConnectedToServer()
{
    if (m_isHost)
    {
        throw std::logic_error("A host cannot be connected to the server (it is the server)");
    }
    else
    {
        return m_client->isConnected();
    }
}

void NetworkManager::sendPlayerMsg(GameMessage &msg)
{
    NetMessage netMsg{NetMessageType::Data, msg};

    if (m_isHost)
    {
        if (m_server)
        {
            m_server->broadcast(netMsg);
        }
    }
    else
    {
        if (m_client)
        {
            m_client->send(netMsg);
        }
    }
}

bool NetworkManager::receiveOpponentMsg(GameMessage &msg)
{
    NetMessage netMsg;
    bool ok = false;

    if (m_isHost)
    {
        if (m_server)
        {
            m_server->recv(m_opponentClientID, netMsg);
        }
    }
    else
    {
        if (m_client)
        {
            m_client->recv(netMsg);
        }
    }

    if (netMsg.header.type == NetMessageType::Data)
    {
        msg = netMsg.body<GameMessage>();
        ok = true;
    }

    return ok;
}

void NetworkManager::shutdown()
{
    if (m_server)
    {
        m_server->shutdown();
    }
    if (m_client)
    {
        m_client->disconnect();
    }
}
