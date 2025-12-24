#pragma once

#include "../networking/tcp/TCPClient.h"
#include "../networking/tcp/TCPMessage.h"

#include "GameMessage.h"


class GameClient
{
    public:
        GameClient()
        {
            m_tcpClient = std::make_unique<TCPClient>();
        }

        ~GameClient()
        {
            m_tcpClient->disconnect();
        }

        bool connectToServer(std::string serverIP, int serverPort)
        {
            bool isConnected = m_tcpClient->connectToServer(serverIP, std::to_string(serverPort));
            return isConnected;
        }

        bool isConnectedToServer()
        {
            return m_tcpClient->isConnected();
        }

        void disconnect()
        {
            m_tcpClient->disconnect();
        }

        void pumpSend(const std::vector<GameMessage>& outMessages)
        {
            for (const auto& outMsg : outMessages)
            {
                sendMessageToServer(outMsg);
            }
        }

        std::vector<GameMessage> pumpReceive()
        {
            std::vector<GameMessage> inMessages;
            GameMessage inMsg;

            while (recieveMessageFromServer(inMsg))
            {
                inMessages.push_back(inMsg);
            }

            return inMessages;
        }

    private:
        void sendMessageToServer(GameMessage msg)
        {
            TCPMessage netMsg{TCPMessageType::Data, msg};
            m_tcpClient->send(netMsg);
        }

        bool recieveMessageFromServer(GameMessage& msg)
        {
            TCPMessage netMsg;
            bool ok = m_tcpClient->recv(netMsg);

            if (!ok || netMsg.header.type != TCPMessageType::Data)
            {
                return false;
            }

            msg = netMsg.body<GameMessage>();
            return true;
        }

        std::unique_ptr<TCPClient> m_tcpClient;
};
