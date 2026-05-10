#include <memory>
#include <thread>
#include <chrono>
#include <csignal>

#include "cli.h"

#include "common/logging.h"

#include "networking/tcp/tcp_listener.h"
#include "networking/udp/udp_transport.h"
#include "networking/utils.h"

#include "game/game_server.h"
#include "game/network_server.h"


std::atomic<bool> running(true);

void signalHandler(int)
{
    running = false;
}

std::unique_ptr<GameServer> initServer(u_int16_t tcpPort,
                                       u_int16_t udpPort,
                                       asio::io_context& ioContext)
{
    auto tcpListener = std::make_shared<TCPListener>(ioContext, tcpPort);
    tcpListener->start();

    auto udpTransport = UDPTransport::create(ioContext);
    udpTransport->openBound(udpPort);

    auto networkServer = std::make_shared<NetworkServer>(tcpListener, udpTransport);

    auto gameServer = std::make_unique<GameServer>(networkServer);
    return gameServer;
}


int main(int argc, char* argv[])
{
    // Configuration
    parseArgs(argc, argv);

    u_int16_t tcpPort = 54000;
    u_int16_t udpPort = 55000;

    try
    {
        LOG_INFO("Initializing server");

        // Start IO context for ASIO async networking tasks
        asio::io_context ioContext;
        IOContextRunner runner(ioContext);

        auto server = initServer(tcpPort,
                                udpPort,
                                ioContext);

        std::signal(SIGINT, signalHandler);


        LOG_INFO("Server initialized, starting update loop");

        auto lastUpdateTime = std::chrono::steady_clock::now();

        while (running)
        {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = now - lastUpdateTime;
            lastUpdateTime = now;

            int deltaTimeMs = static_cast<int>(
                std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()
            );

            server->update(deltaTimeMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("%s", e.what());
        return 1;
    }

    LOG_INFO("Server shutting down");
    return 0;
}
