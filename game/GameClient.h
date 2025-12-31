#pragma once

#include "../common/Constants.h"
#include "../common/Utils.h"

#include "NetworkClient.h"
#include "RenderSystem.h"
#include "InputSystem.h"
#include "GameSimulation.h"
#include "FrameTimer.h"
#include "StatsManager.h"


class GameClient
{
    public:
        struct Config
        {
            Constants::TransportType transportForPlayerStateUpdates = Constants::TransportType::TCP;

            int assignPlayerIDTimeoutMs      = 3000;
            int assignPlayerIDPollIntervalMs = 50;
        };

    public:
        GameClient(Config config,
                   std::shared_ptr<NetworkClient> networkClient)
        : m_config(config)
        , m_networkClient(networkClient)
        {
            assert(m_networkClient != nullptr);
        }

        void init()
        {
            if (!m_renderSystem.init())
            {
                throw std::runtime_error("Failed to initialize render system");
            }

            m_networkClient->doUDPHandshake();

            if (!m_networkClient->isConnected())
            {
                throw std::runtime_error("Network client failed to connect to server");
            }

            bool assignedPlayerID = waitToAssignLocalPlayerID();

            if (!assignedPlayerID)
            {
                throw std::runtime_error("Timed out waiting for AssignLocalPlayerID");
            }

            m_gameSimulation.addLocalPlayer(m_localPlayerID);
        }

        bool isConnected() const
        {
            return m_networkClient->isConnected();
        }

        void run()
        {
            constexpr int GAME_TICK_RATE_MS = 16; // ~16 ms per frame (~60 updates per second)

            bool shouldQuit = false;
            FrameTimer frameTimer(GAME_TICK_RATE_MS);

            int remotePlayerUpdatesThisFrame = 0;

            std::vector<Message> inMessages{};

            while (!shouldQuit && isConnected())
            {
                frameTimer.startFrame();

                auto incomingMessages = pumpReceive();

                auto events = m_inputSystem.poll();

                for (const auto& event : events)
                {
                    if (auto appEvent = std::get_if<AppEvent>(&event))
                    {
                        if (*appEvent == AppEvent::Quit)
                        {
                            shouldQuit = true;
                        }
                    }
                    else if (auto gameEvent = std::get_if<GameEvent>(&event))
                    {
                        m_gameSimulation.applyLocalInput(*gameEvent);
                    }
                }

                for (const auto& inMessage : incomingMessages)
                {
                    if (inMessage.type == MessageType::PlayerStateUpdate)
                    {
                        remotePlayerUpdatesThisFrame++;
                    }
                    m_gameSimulation.applyIncomingMessage(inMessage);
                }

                m_gameSimulation.updateSimulation(frameTimer.getDeltaTime());

                auto outgoingMessages = m_gameSimulation.collectOutgoingMessages();
                pumpSend(outgoingMessages);

                m_statsManager.pushUpdatesPerFrameSample(remotePlayerUpdatesThisFrame);

                m_renderSystem.renderGame(m_gameSimulation,
                                          m_networkClient->getPingMs(),
                                          m_statsManager.computeUpdatesPerFrameCV(),
                                          m_config.transportForPlayerStateUpdates);

                frameTimer.endFrame();
                remotePlayerUpdatesThisFrame = 0; // reset count
            }
        }

    private:
        bool waitToAssignLocalPlayerID()
        {
            int elapsed = 0;

            while (elapsed < m_config.assignPlayerIDTimeoutMs)
            {
                m_networkClient->pumpReceive();
                auto messages = m_networkClient->consumeIncomingMessages();

                for (const auto& msg : messages)
                {
                    if (msg.type == MessageType::AssignLocalPlayerID)
                    {
                        m_localPlayerID = msg.data.assignLocalPlayerID.playerID;

                        LOG_INFO("Received player ID from server (id=%u)",
                                 m_localPlayerID);

                        return true;
                    }
                }

                sleepMs(m_config.assignPlayerIDPollIntervalMs);
                elapsed += m_config.assignPlayerIDPollIntervalMs;
            }

            return false;
        }

        void pumpSend(const std::vector<Message>& outMessages)
        {
            for (const auto& outMsg : outMessages)
            {
                if (outMsg.type == MessageType::PlayerStateUpdate)
                {
                    m_networkClient->queueOutgoingMessage(
                        outMsg, m_config.transportForPlayerStateUpdates
                    );
                }
                else
                {
                    m_networkClient->queueOutgoingMessage(
                        outMsg, Constants::TransportType::TCP
                    );
                }
            }

            m_networkClient->pumpSend();
        }

        std::vector<Message> pumpReceive()
        {
            m_networkClient->pumpReceive();
            return m_networkClient->consumeIncomingMessages();
        }

    private:
        std::shared_ptr<NetworkClient> m_networkClient;
        RenderSystem m_renderSystem;
        GameSimulation m_gameSimulation;
        InputSystem m_inputSystem;
        StatsManager m_statsManager;

        PlayerID m_localPlayerID;

        Config m_config;
};
