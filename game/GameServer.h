#pragma once

#include "../networking/tcp/TCPServer.h"
#include "../networking/tcp/TCPMessage.h"

#include "GameMessage.h"


class GameServer
{
    public:
        GameServer(int port)
        {
            m_tcpServer = std::make_unique<TCPServer>(port);

            m_tcpServer->setOnClientConnect([this](ClientID clientID) {
                onClientConnected(clientID);
            });
            m_tcpServer->setOnClientDisconnect([this](ClientID clientID) {
                onClientDisconnected(clientID);
            });
        }

        ~GameServer()
        {
            m_tcpServer->shutdown();
        }

        void start()
        {
            m_tcpServer->start();
        }

        void shutdown()
        {
            m_tcpServer->shutdown();
        }

        void pump()
        {
            // Relay PlayerState updates from one player to every player,
            // for all players.
            //
            // Take a snapshot of the clientIDs. Seems to prevent seg faults.
            // TODO: Investigate
            auto clientIDsSnapshot = m_tcpServer->getClientIDs();
            for (const auto& clientID : clientIDsSnapshot)
            {
                GameMessage gameMsg;

                while (recieveMessage(clientID, gameMsg))
                {
                    if (gameMsg.type == GameMessageType::PlayerStateUpdate)
                    {
                        broadcastMessage(gameMsg, clientID);
                    }
                }
            }
        }

    private:
        void onClientConnected(ClientID connectedClientID)
        {
            GameMessage welcomeMsg{
                GameMessageType::Welcome,
                {
                    .welcome = {
                        connectedClientID
                    }
                }
            };

            GameMessage playerJoinedMsg{
                GameMessageType::PlayerJoined,
                {
                    .playerJoined = {
                        connectedClientID
                    }
                }
            };

            sendMessage(connectedClientID, welcomeMsg);
            broadcastMessage(playerJoinedMsg, connectedClientID);
        };

        void onClientDisconnected(ClientID disconnectedClientID)
        {
            GameMessage playerLeftMsg{
                GameMessageType::PlayerLeft,
                {
                    .playerLeft = {
                        disconnectedClientID
                    }
                }
            };

            broadcastMessage(playerLeftMsg, disconnectedClientID);
        };

        void sendMessage(ClientID clientID, GameMessage msg)
        {
            TCPMessage netMsg{TCPMessageType::Data, msg};
            m_tcpServer->send(clientID, netMsg);
        }

        void broadcastMessage(GameMessage msg, std::optional<ClientID> ignoreClientID)
        {
            TCPMessage netMsg{TCPMessageType::Data, msg};
            m_tcpServer->broadcast(netMsg, ignoreClientID);
        }

        bool recieveMessage(ClientID clientID, GameMessage& msg)
        {
            TCPMessage netMsg;
            bool ok = m_tcpServer->recv(clientID, netMsg);

            if (!ok || netMsg.header.type != TCPMessageType::Data)
            {
                return false;
            }

            msg = netMsg.body<GameMessage>();
            return true;
        }

        std::unique_ptr<TCPServer> m_tcpServer;
};
