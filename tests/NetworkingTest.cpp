#include <gtest/gtest.h>

#include "../networking/NetClient.h"
#include "../networking/NetServer.h"
#include "../networking/NetMessages.h"


TEST(NetworkingTest, ClientsConnectToServer)
{
    std::vector<std::unique_ptr<NetClient>> clients;

    for (int i = 0; i < 5; i++)
    {
        clients.push_back(std::make_unique<NetClient>());
    }

    std::unique_ptr<NetServer> server = std::make_unique<NetServer>(8080);

    server->start();

    for (auto &client : clients)
    {
        client->connectToServer("127.0.0.1", "8080");
    }

    NetMessage msg;

    for (auto &client : clients)
    {
        ASSERT_EQ(client->blockingRecv(msg), true);
        ASSERT_EQ(msg.header.type, NetMessageType::ConnectOk);
    }

    for (auto &client : clients)
    {
        ASSERT_EQ(client->isConnected(), true);
    }
}

TEST(NetworkingTest, ClientDisconectsFromServer)
{
    std::unique_ptr<NetClient> client = std::make_unique<NetClient>();
    std::unique_ptr<NetServer> server = std::make_unique<NetServer>(8080);

    server->start();
    client->connectToServer("127.0.0.1", "8080");

    NetMessage inMsg;

    ASSERT_EQ(client->blockingRecv(inMsg), true);
    ASSERT_EQ(inMsg.header.type, NetMessageType::ConnectOk);
    ASSERT_EQ(client->isConnected(), true);

    server->shutdown();

    ASSERT_EQ(client->blockingRecv(inMsg), true);
    ASSERT_EQ(inMsg.header.type, NetMessageType::Disconnect);
    ASSERT_EQ(client->isConnected(), false);
}

TEST(NetworkingTest, ClientSendsMessagesToServer)
{
    std::unique_ptr<NetClient> client = std::make_unique<NetClient>();
    std::unique_ptr<NetServer> server = std::make_unique<NetServer>(8080);

    int numMsgsToSend = 10000;

    server->start();
    client->connectToServer("127.0.0.1", "8080");

    NetMessage inMsg;

    ASSERT_EQ(client->blockingRecv(inMsg), true);
    ASSERT_EQ(inMsg.header.type, NetMessageType::ConnectOk);

    int clientID = inMsg.body<ConnectOkBody>().assignedClientID;

    std::vector<NetMessage> outMsgs;
    for (int i = 0; i < numMsgsToSend; i++)
    {
        outMsgs.push_back(NetMessage{NetMessageType::Data, 100 + i});
    }

    for (auto &msg : outMsgs)
    {
        client->send(msg);
    }

    for (int i = 0; i < numMsgsToSend; i++)
    {
        ASSERT_EQ(server->blockingRecv(clientID, inMsg), true);
        ASSERT_EQ(inMsg.header.type, NetMessageType::Data);
        ASSERT_EQ(inMsg.body<int>(), 100 + i);
    }
}

TEST(NetworkingTest, ServerSendsMessagesToClient)
{
    bool success = true;

    std::unique_ptr<NetClient> client = std::make_unique<NetClient>();
    std::unique_ptr<NetServer> server = std::make_unique<NetServer>(8080);

    int numMsgsToSend = 10000;

    server->start();
    client->connectToServer("127.0.0.1", "8080");

    NetMessage inMsg;

    ASSERT_EQ(client->blockingRecv(inMsg), true);
    ASSERT_EQ(inMsg.header.type, NetMessageType::ConnectOk);

    int clientID = inMsg.body<ConnectOkBody>().assignedClientID;

    std::vector<NetMessage> outMsgs;

    for (int i = 0; i < numMsgsToSend; i++)
    {
        outMsgs.push_back(NetMessage{NetMessageType::Data, 100 + i});
    }

    for (auto &msg : outMsgs)
    {
        server->send(clientID, msg);
    }

    for (int i = 0; i < numMsgsToSend; i++)
    {
        ASSERT_EQ(client->blockingRecv(inMsg), true);
        ASSERT_EQ(inMsg.header.type, NetMessageType::Data);
        ASSERT_EQ(inMsg.body<int>(), 100 + i);
    }
}

TEST(NetworkingTest, ClientConnectsToUnknownHost)
{
    std::unique_ptr<NetClient> client = std::make_unique<NetClient>();

    // Server shouldn't be running at 127.0.0.1:9090
    ASSERT_EQ(client->connectToServer("127.0.0.1", "9090"), false);
    ASSERT_EQ(client->isConnected(), false);
}

TEST(NetworkingTest, ClientSendFailsWhenNotConnected)
{
    std::unique_ptr<NetClient> client = std::make_unique<NetClient>();

    NetMessage inMsg;
    ASSERT_EQ(client->send(inMsg), false);
}

TEST(NetworkingTest, ClientReceiveFailsWhenNotConnected)
{
    std::unique_ptr<NetClient> client = std::make_unique<NetClient>();

    NetMessage inMsg;
    ASSERT_EQ(client->recv(inMsg), false);
    ASSERT_EQ(client->blockingRecv(inMsg), false);
}

TEST(NetworkingTest, ServerSendFailsForUnconnectedClient)
{
    std::unique_ptr<NetServer> server = std::make_unique<NetServer>(8080);

    int randomClientID = 123;

    NetMessage inMsg;
    ASSERT_EQ(server->send(randomClientID, inMsg), false);
}

TEST(NetworkingTest, ServerReceiveFailsForUnconnectedClient)
{
    std::unique_ptr<NetServer> server = std::make_unique<NetServer>(8080);

    int randomClientID = 123;

    NetMessage inMsg;
    ASSERT_EQ(server->recv(randomClientID, inMsg), false);
    ASSERT_EQ(server->blockingRecv(randomClientID, inMsg), false);
}
