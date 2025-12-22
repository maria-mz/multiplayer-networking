#pragma once

#include "GameClient.h"
#include "GameMessage.h"
#include "GameSimulation.h"
#include "RenderSystem.h"
#include "InputSystem.h"
#include "FrameTimer.h"
#include "../utils/Utils.h"

class App
{
    public:
        App(std::string serverIP, int serverPort)
            : m_serverIP(serverIP)
            , m_serverPort(serverPort)
        {}

        bool start()
        {
            if (!m_renderSystem.init())
            {
                throw std::runtime_error("Failed to initialize render system");
            }

            if (!m_gameClient.connectToServer(m_serverIP, m_serverPort))
            {
                throw std::runtime_error("Failed to connect to game server");
            }

            bool started = waitForWelcome();
            return started;
        }

        void run()
        {
            constexpr int GAME_TICK_RATE_MS = 16; // ~16 ms per frame (~60 updates per second)

            bool shouldQuit = false;
            FrameTimer frameTimer(GAME_TICK_RATE_MS);

            std::vector<GameMessage> inMessages{};

            while (!shouldQuit && m_gameClient.isConnectedToServer())
            {
                frameTimer.startFrame();

                auto incomingMessages = m_gameClient.pumpReceive();

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

                m_gameSimulation.applyIncomingMessages(incomingMessages);
                m_gameSimulation.updateSimulation(frameTimer.getDeltaTime());

                auto outgoingMessages = m_gameSimulation.collectOutgoingMessages();
                m_gameClient.pumpSend(outgoingMessages);

                m_renderSystem.renderGame(m_gameSimulation);

                frameTimer.endFrame();
            }
        }

    private:
        bool waitForWelcome()
        {
            constexpr int TIMEOUT_MS = 3000;
            constexpr int POLL_INTERVAL_MS = 50;

            int elapsed = 0;

            while (elapsed < TIMEOUT_MS)
            {
                auto messages = m_gameClient.pumpReceive();

                for (const auto& msg : messages)
                {
                    if (msg.type == GameMessageType::Welcome)
                    {
                        PlayerID localID = msg.data.welcome.playerID;
                        m_gameSimulation.addLocalPlayer(localID);

                        LOG_INFO("Received welcome. Local player ID = %u", localID);
                        return true;
                    }
                }

                sleepMs(POLL_INTERVAL_MS);
                elapsed += POLL_INTERVAL_MS;
            }

            LOG_ERROR("Timed out waiting for welcome message");
            return false;
        }

    private:
        std::string m_serverIP;
        int m_serverPort;

        RenderSystem m_renderSystem;
        GameClient m_gameClient;
        GameSimulation m_gameSimulation;
        InputSystem m_inputSystem;
};
