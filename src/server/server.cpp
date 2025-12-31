#include <memory>
#include <thread>
#include <chrono>
#include <csignal>

#include "cli.h"

#include "../../common/Logging.h"

#include "../../networking/tcp/TCPListener.h"
#include "../../networking/udp/UDPTransport.h"
#include "../../networking/NetworkUtils.h"

#include "../../game/GameServer.h"
#include "../../game/NetworkServer.h"


std::atomic<bool> running(true);

void signalHandler(int)
{
    running = false;
}

std::unique_ptr<GameServer> initServer(u_int16_t tcpPort,
                                       u_int16_t udpPort,
                                       GameServer::Config gameServerConfig,
                                       asio::io_context& ioContext)
{
    auto tcpListener = std::make_shared<TCPListener>(ioContext, tcpPort);
    tcpListener->start();

    auto udpTransport = UDPTransport::create(ioContext);
    udpTransport->openBound(udpPort);

    auto networkServer = std::make_shared<NetworkServer>(tcpListener, udpTransport);

    auto gameServer = std::make_unique<GameServer>(gameServerConfig, networkServer);
    return gameServer;
}


int main(int argc, char* argv[])
{
    // Configuration
    CLIOptions opts = parseArgs(argc, argv);

    u_int16_t tcpPort = 54000;
    u_int16_t udpPort = 55000;

    GameServer::Config gameServerConfig{
        // Must match client's transport type
        .transportForPlayerStateUpdates = opts.transport
    };


    LOG_INFO("Initializing server");

    // Start IO context for ASIO async networking tasks
    asio::io_context ioContext;
    IOContextRunner runner(ioContext);

    auto server = initServer(tcpPort,
                             udpPort,
                             gameServerConfig,
                             ioContext);

    std::signal(SIGINT, signalHandler);


    LOG_INFO("Server initialized, starting update loop");

    while (running)
    {
        server->update();
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }


    LOG_INFO("Server shutting down");
    return 0;
}
