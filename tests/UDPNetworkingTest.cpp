#include <gtest/gtest.h>
#include <thread>
#include <chrono>

#include "../networking/UDPServer.h"
#include "../networking/UDPClient.h"

// Helper function that waits for a condition or times out
bool waitFor(std::function<bool()> condition, int timeoutMs = 200)
{
    auto start = std::chrono::steady_clock::now();
    while (!condition())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        if (std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count() > timeoutMs)
            return false;
    }
    return true;
}

TEST(UDPNetworkingTest, ClientToServer)
{
    int serverPort = 8080;
    UDPServer server(serverPort);
    server.start();

    UDPClient client("127.0.0.1", serverPort);
    client.start();

    UDPPacket pkt(123);
    client.send(pkt);

    UDPPacket received;
    asio::ip::udp::endpoint sender;

    bool ok = waitFor([&]() { return server.recv(received, sender); });

    ASSERT_TRUE(ok);
    ASSERT_EQ(received.data<int>(), 123);
}

TEST(UDPNetworkingTest, ServerToClient)
{
    int serverPort = 8080;
    UDPServer server(serverPort);
    server.start();

    UDPClient client("127.0.0.1", serverPort);
    client.start();

    asio::ip::udp::endpoint clientEndpoint(
        asio::ip::address::from_string("127.0.0.1"),
        client.getLocalPort()
    );

    UDPPacket pkt(123);
    server.sendTo(pkt, clientEndpoint);

    UDPPacket received;
    bool ok = waitFor([&]() { return client.recv(received); });

    ASSERT_TRUE(ok);
    ASSERT_EQ(received.data<int>(), 123);
}

TEST(UDPNetworkingTest, MultipleClients)
{
    int serverPort = 8080;
    UDPServer server(serverPort);
    server.start();

    UDPClient client1("127.0.0.1", serverPort);
    UDPClient client2("127.0.0.1", serverPort);
    client1.start();
    client2.start();

    UDPPacket pkt1(100);
    UDPPacket pkt2(200);
    client1.send(pkt1);
    client2.send(pkt2);

    std::vector<int> receivedMsgs;
    for (int i = 0; i < 2; ++i)
    {
        UDPPacket rcv;
        asio::ip::udp::endpoint sender;
        bool ok = waitFor([&]() { return server.recv(rcv, sender); });
        ASSERT_TRUE(ok);
        receivedMsgs.push_back(rcv.data<int>());
    }

    ASSERT_NE(std::find(receivedMsgs.begin(), receivedMsgs.end(), 100), receivedMsgs.end());
    ASSERT_NE(std::find(receivedMsgs.begin(), receivedMsgs.end(), 200), receivedMsgs.end());
}

TEST(UDPNetworkingTest, CleanShutdown)
{
    int serverPort = 8080;
    UDPServer server(serverPort);
    server.start();

    UDPClient client("127.0.0.1", serverPort);
    client.start();

    ASSERT_NO_THROW({
        client.shutdown();
        server.shutdown();
    });
}

TEST(UDPNetworkingTest, ManyMessages)
{
    const int numMessages = 1000;
    const int serverPort = 8080;

    UDPServer server(serverPort);
    server.start();

    UDPClient client("127.0.0.1", serverPort);
    client.start();

    for (int i = 0; i < numMessages; ++i)
    {
        UDPPacket pkt(i);
        client.send(pkt);
    }

    std::vector<int> received;
    for (int i = 0; i < numMessages; ++i)
    {
        UDPPacket pkt;
        asio::ip::udp::endpoint sender;
        if (waitFor([&]() { return server.recv(pkt, sender); }, 50))
        {
            received.push_back(pkt.data<int>());
        }
    }

    std::cout << "Sent " << numMessages << " packets, received " << received.size() << std::endl;

    // Expect at least 95% received
    ASSERT_GE(received.size(), numMessages * 0.95);

    client.shutdown();
    server.shutdown();
}
