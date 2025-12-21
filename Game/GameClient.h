#pragma once

#include "../Networking/NetClient.h"
#include "../Networking/NetMessages.h"

#include "GameMessage.h"


class GameClient
{
    public:
        GameClient()
        {
            m_netClient = std::make_unique<NetClient>();
        }

        ~GameClient()
        {
            m_netClient->disconnect();
        }

        bool connectToServer(std::string serverIP, int serverPort)
        {
            bool isConnected = m_netClient->connectToServer(serverIP, std::to_string(serverPort));

            return isConnected;
        }

        bool isConnectedToServer()
        {
            return m_netClient->isConnected();
        }

        void disconnect()
        {
            m_netClient->disconnect();
        }

        void sendMessagesToServer(const std::vector<GameMessage>& outMessages)
        {
            for (const auto& outMsg : outMessages)
            {
                sendMessageToServer(outMsg);
            }
        }

        std::vector<GameMessage> recvMessagesFromServer()
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
            NetMessage netMsg{NetMessageType::Data, msg};
            m_netClient->send(netMsg);
        }

        bool recieveMessageFromServer(GameMessage& msg)
        {
            NetMessage netMsg;
            bool ok = m_netClient->recv(netMsg);

            if (!ok || netMsg.header.type != NetMessageType::Data)
            {
                return false;
            }

            msg = netMsg.body<GameMessage>();
            return true;
        }

        std::unique_ptr<NetClient> m_netClient;
};
