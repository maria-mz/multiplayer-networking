#pragma once

#include "../networking/NetServer.h"
#include "../networking/NetMessages.h"

#include "GameMessage.h"


class GameServer
{
    public:
        GameServer(int port)
        {
            m_netServer = std::make_unique<NetServer>(port);

            m_netServer->setOnClientConnect([this](ClientID clientID) {
                onClientConnected(clientID);
            });
            m_netServer->setOnClientDisconnect([this](ClientID clientID) {
                onClientDisconnected(clientID);
            });
        }

        ~GameServer()
        {
            m_netServer->shutdown();
        }

        void start()
        {
            m_netServer->start();
        }

        void shutdown()
        {
            m_netServer->shutdown();
        }

        void pump()
        {
            // Relay PlayerState updates from one player to every player,
            // for all players.
            for (const auto& clientID : m_netServer->getClientIDs())
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
            NetMessage netMsg{NetMessageType::Data, msg};
            m_netServer->send(clientID, netMsg);
        }

        void broadcastMessage(GameMessage msg, std::optional<ClientID> ignoreClientID)
        {
            NetMessage netMsg{NetMessageType::Data, msg};
            m_netServer->broadcast(netMsg, ignoreClientID);
        }

        bool recieveMessage(ClientID clientID, GameMessage& msg)
        {
            NetMessage netMsg;
            bool ok = m_netServer->recv(clientID, netMsg);

            if (!ok || netMsg.header.type != NetMessageType::Data)
            {
                return false;
            }

            msg = netMsg.body<GameMessage>();
            return true;
        }

        std::unique_ptr<NetServer> m_netServer;
};
