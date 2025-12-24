#include <gtest/gtest.h>

#include "../networking/tcp/TCPClient.h"
#include "../networking/tcp/TCPServer.h"


TEST(TCPNetworkingTest, ClientsConnectToServer)
{
    std::vector<std::unique_ptr<TCPClient>> clients;

    for (int i = 0; i < 5; i++)
    {
        clients.push_back(std::make_unique<TCPClient>());
    }

    std::unique_ptr<TCPServer> server = std::make_unique<TCPServer>(8080);

    server->start();

    for (auto &client : clients)
    {
        client->connectToServer("127.0.0.1", "8080");
    }

    TCPMessage msg;

    for (auto &client : clients)
    {
        ASSERT_EQ(client->blockingRecv(msg), true);
        ASSERT_EQ(msg.header.type, TCPMessageType::ConnectOk);
    }

    for (auto &client : clients)
    {
        ASSERT_EQ(client->isConnected(), true);
    }
}

TEST(TCPNetworkingTest, ClientDisconectsFromServer)
{
    std::unique_ptr<TCPClient> client = std::make_unique<TCPClient>();
    std::unique_ptr<TCPServer> server = std::make_unique<TCPServer>(8080);

    server->start();
    client->connectToServer("127.0.0.1", "8080");

    TCPMessage inMsg;

    ASSERT_EQ(client->blockingRecv(inMsg), true);
    ASSERT_EQ(inMsg.header.type, TCPMessageType::ConnectOk);
    ASSERT_EQ(client->isConnected(), true);

    server->shutdown();

    ASSERT_EQ(client->blockingRecv(inMsg), true);
    ASSERT_EQ(inMsg.header.type, TCPMessageType::Disconnect);
    ASSERT_EQ(client->isConnected(), false);
}

TEST(TCPNetworkingTest, ClientSendsMessagesToServer)
{
    std::unique_ptr<TCPClient> client = std::make_unique<TCPClient>();
    std::unique_ptr<TCPServer> server = std::make_unique<TCPServer>(8080);

    int numMsgsToSend = 10000;

    server->start();
    client->connectToServer("127.0.0.1", "8080");

    TCPMessage inMsg;

    ASSERT_EQ(client->blockingRecv(inMsg), true);
    ASSERT_EQ(inMsg.header.type, TCPMessageType::ConnectOk);

    int clientID = inMsg.body<ConnectOkBody>().assignedClientID;

    std::vector<TCPMessage> outMsgs;
    for (int i = 0; i < numMsgsToSend; i++)
    {
        outMsgs.push_back(TCPMessage{TCPMessageType::Data, 100 + i});
    }

    for (auto &msg : outMsgs)
    {
        client->send(msg);
    }

    for (int i = 0; i < numMsgsToSend; i++)
    {
        ASSERT_EQ(server->blockingRecv(clientID, inMsg), true);
        ASSERT_EQ(inMsg.header.type, TCPMessageType::Data);
        ASSERT_EQ(inMsg.body<int>(), 100 + i);
    }
}

TEST(TCPNetworkingTest, ServerSendsMessagesToClient)
{
    bool success = true;

    std::unique_ptr<TCPClient> client = std::make_unique<TCPClient>();
    std::unique_ptr<TCPServer> server = std::make_unique<TCPServer>(8080);

    int numMsgsToSend = 10000;

    server->start();
    client->connectToServer("127.0.0.1", "8080");

    TCPMessage inMsg;

    ASSERT_EQ(client->blockingRecv(inMsg), true);
    ASSERT_EQ(inMsg.header.type, TCPMessageType::ConnectOk);

    int clientID = inMsg.body<ConnectOkBody>().assignedClientID;

    std::vector<TCPMessage> outMsgs;

    for (int i = 0; i < numMsgsToSend; i++)
    {
        outMsgs.push_back(TCPMessage{TCPMessageType::Data, 100 + i});
    }

    for (auto &msg : outMsgs)
    {
        server->send(clientID, msg);
    }

    for (int i = 0; i < numMsgsToSend; i++)
    {
        ASSERT_EQ(client->blockingRecv(inMsg), true);
        ASSERT_EQ(inMsg.header.type, TCPMessageType::Data);
        ASSERT_EQ(inMsg.body<int>(), 100 + i);
    }
}

TEST(TCPNetworkingTest, ClientConnectsToUnknownHost)
{
    std::unique_ptr<TCPClient> client = std::make_unique<TCPClient>();

    // Server shouldn't be running at 127.0.0.1:9090
    ASSERT_EQ(client->connectToServer("127.0.0.1", "9090"), false);
    ASSERT_EQ(client->isConnected(), false);
}

TEST(TCPNetworkingTest, ClientSendFailsWhenNotConnected)
{
    std::unique_ptr<TCPClient> client = std::make_unique<TCPClient>();

    TCPMessage inMsg;
    ASSERT_EQ(client->send(inMsg), false);
}

TEST(TCPNetworkingTest, ClientReceiveFailsWhenNotConnected)
{
    std::unique_ptr<TCPClient> client = std::make_unique<TCPClient>();

    TCPMessage inMsg;
    ASSERT_EQ(client->recv(inMsg), false);
    ASSERT_EQ(client->blockingRecv(inMsg), false);
}

TEST(TCPNetworkingTest, ServerSendFailsForUnconnectedClient)
{
    std::unique_ptr<TCPServer> server = std::make_unique<TCPServer>(8080);

    int randomClientID = 123;

    TCPMessage inMsg;
    ASSERT_EQ(server->send(randomClientID, inMsg), false);
}

TEST(TCPNetworkingTest, ServerReceiveFailsForUnconnectedClient)
{
    std::unique_ptr<TCPServer> server = std::make_unique<TCPServer>(8080);

    int randomClientID = 123;

    TCPMessage inMsg;
    ASSERT_EQ(server->recv(randomClientID, inMsg), false);
    ASSERT_EQ(server->blockingRecv(randomClientID, inMsg), false);
}
