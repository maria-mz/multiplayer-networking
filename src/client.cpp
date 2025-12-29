#include <memory>
#include <thread>
#include <chrono>
#include <csignal>

#include "../common/Logging.h"
#include "../networking/tcp/TCPConnection.h"
#include "../networking/udp/UDPTransport.h"
#include "../networking/NetworkUtils.h"

#include "../game/GameClient.h"
#include "../game/NetworkClient.h"


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
    std::string serverIPAddress = "127.0.0.1";
    u_int16_t serverTCPPort = 54000;
    u_int16_t serverUDPPort = 55000;

    asio::ip::tcp::endpoint serverTCPEndpoint(
        asio::ip::make_address(serverIPAddress), serverTCPPort
    );
    asio::ip::udp::endpoint serverUDPEndpoint(
        asio::ip::make_address(serverIPAddress), serverUDPPort
    );

    NetworkClient::Config networkClientConfig{
        .serverUDPEndpoint = serverUDPEndpoint
    };

    GameClient::Config gameClientConfig{
        // NOTE: Must match server's transport type
        .transportForPlayerStateUpdates = Constants::TransportType::TCP
    };


    LOG_INFO("Initializing client");

    // Start IO context for ASIO async networking tasks
    asio::io_context ioContext;
    IOContextRunner runner(ioContext);

    auto client = initClient(serverTCPEndpoint,
                             networkClientConfig,
                             gameClientConfig,
                             ioContext);


    LOG_INFO("Client initialized, calling run()");
    client->run();


    LOG_INFO("Client shutting down");
    return 0;
}
