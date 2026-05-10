#pragma once

#include "common/constants.h"
#include "common/utils.h"

#include "network_client.h"
#include "render_system.h"
#include "input_system.h"
#include "game_simulation.h"
#include "frame_timer.h"
#include "stats_manager.h"


class GameClient
{
    public:
        struct Config
        {
            int assignPlayerIDTimeoutMs      = 3000;
            int assignPlayerIDPollIntervalMs = 50;
        };

    public:
        GameClient(Config config,
                   std::shared_ptr<NetworkClient> networkClient)
        : m_config(config)
        , m_networkClient(networkClient)
        , m_gameSimulation(GameSimulation::Config{})
        , m_showDebugUI(false)
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

            m_gameSimulation.addPlayer(m_localPlayerID);
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
                        else if (*appEvent == AppEvent::ToggleDebugUI)
                        {
                            m_showDebugUI = !m_showDebugUI;
                        }
                    }
                    else if (auto gameEvent = std::get_if<GameEvent>(&event))
                    {
                        m_gameSimulation.applyPlayerInput(m_localPlayerID, *gameEvent);
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

                m_gameSimulation.updateSimulation(frameTimer.getDeltaTime(), m_localPlayerID);

                auto outgoingMessages = m_gameSimulation.collectOutgoingMessages();
                outgoingMessages.push_back(
                    Message{
                        .type = MessageType::PlayerStateUpdate,
                        .data = { .playerStateUpdate = m_gameSimulation.makePlayerStateUpdate(m_localPlayerID) }
                    }
                );
                pumpSend(outgoingMessages);

                m_statsManager.pushRemoteUpdateVariabilitySample(remotePlayerUpdatesThisFrame);

                m_renderSystem.renderGame(m_gameSimulation,
                                          m_networkClient->getPingMs(),
                                          m_statsManager.computeRemoteUpdateVariability(),
                                          m_showDebugUI);

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
                        outMsg, constants::TransportType::UDP
                    );
                }
                else
                {
                    m_networkClient->queueOutgoingMessage(
                        outMsg, constants::TransportType::TCP
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
        bool m_showDebugUI;

        Config m_config;
};
