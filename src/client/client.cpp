#include <memory>
#include <thread>
#include <chrono>
#include <csignal>

#include "cli.h"

#include "common/Logging.h"
#include "networking/tcp/TCPConnection.h"
#include "networking/udp/UDPTransport.h"
#include "networking/NetworkUtils.h"

#include "game/GameClient.h"
#include "game/NetworkClient.h"


std::unique_ptr<GameClient> initClient(asio::ip::tcp::endpoint serverTCPEndpoint,
                                       NetworkClient::Config networkClientConfig,
                                       GameClient::Config gameClientConfig,
                                       asio::io_context& ioContext)
{
    auto tcpConnection = establishTCPConnection(ioContext, serverTCPEndpoint);

    auto udpTransport = UDPTransport::create(ioContext);
    udpTransport->openEphemeral();

    auto networkClient = std::make_shared<NetworkClient>(
        tcpConnection, udpTransport, networkClientConfig
    );

    auto client = std::make_unique<GameClient>(gameClientConfig, networkClient);

    client->init();

    return client;
}


int main(int argc, char* argv[])
{
    // Configuration
    CLIOptions opts = parseArgs(argc, argv);

    std::string serverIPAddress = "127.0.0.1";
    uint16_t serverTCPPort = 54000;
    uint16_t serverUDPPort = 55000;

    asio::ip::tcp::endpoint serverTCPEndpoint(
        asio::ip::make_address(serverIPAddress), serverTCPPort
    );
    asio::ip::udp::endpoint serverUDPEndpoint(
        asio::ip::make_address(serverIPAddress), serverUDPPort
    );

    NetworkClient::Config networkClientConfig{
        .serverUDPEndpoint = serverUDPEndpoint,
        .pingTransport = opts.transport
    };

    GameClient::Config gameClientConfig{
        // NOTE: Must match server's transport type
        .transportForPlayerStateUpdates = opts.transport
    };


    LOG_INFO("Initializing client");

    // Start IO context for ASIO async networking tasks
    asio::io_context ioContext;
    IOContextRunner runner(ioContext);

    try
    {
        auto client = initClient(serverTCPEndpoint,
                                networkClientConfig,
                                gameClientConfig,
                                ioContext);

        LOG_INFO("Client initialized, calling run()");
        client->run();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("%s", e.what());
        return 1;
    }

    LOG_INFO("Client shutting down");
    return 0;
}
