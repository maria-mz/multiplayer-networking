#include <gtest/gtest.h>

#include "game/network_server.h"
#include "utils.h"


class NetworkServerTest : public ::testing::Test
{
protected:
    const asio::ip::udp::endpoint TEST_UDP_ENDPOINT_1{
        asio::ip::make_address("127.0.0.1"), 40000
    };
    const asio::ip::udp::endpoint TEST_UDP_ENDPOINT_2{
        asio::ip::make_address("127.0.0.1"), 50000
    };

    void SetUp() override
    {
        tcpListener = std::make_shared<MockTCPListener>();
        udpTransport = std::make_shared<MockUDPTransport>();
        server = std::make_unique<NetworkServer>(tcpListener, udpTransport);
    }

    // Created mock TCP connection is an out parameter because ASSERTs can
    // only be used in void functions. The ASSERTs are useful as they verify
    // that the connection behaves as expected.
    void connectClient(const asio::ip::udp::endpoint& udpEndpoint,
                       std::shared_ptr<MockTCPConnection>& outConn)
    {
        auto tcpConn = std::make_shared<MockTCPConnection>();
        tcpListener->nextConnections.push(tcpConn);

        server->acceptConnections();
        server->pumpSend();

        ASSERT_EQ(tcpConn->sent.size(), 1u);

        Message udpBindRequest = fromTCPMessage(tcpConn->sent[0]);
        ASSERT_EQ(udpBindRequest.type, MessageType::UDPBindRequest);
        ASSERT_NE(udpBindRequest.data.udpBindRequest.token, 0);

        Message udpBind{
            .type = MessageType::UDPBind,
            .data = { .udpBind = { .token = udpBindRequest.data.udpBindRequest.token } }
        };

        udpTransport->received.push({ UDPMessage{udpBind}, udpEndpoint });

        server->pumpReceive();
        server->pumpSend();

        ASSERT_EQ(tcpConn->sent.size(), 2u);

        Message udpBindAck = fromTCPMessage(tcpConn->sent[1]);
        ASSERT_EQ(udpBindAck.type, MessageType::UDPBindAck);

        tcpConn->sent.clear();
        outConn = tcpConn;
    }

    std::shared_ptr<MockTCPListener> tcpListener;
    std::shared_ptr<MockUDPTransport> udpTransport;
    std::unique_ptr<NetworkServer> server;
};

TEST_F(NetworkServerTest, ClientCanConnect)
{
    std::shared_ptr<MockTCPConnection> tcpConn;
    connectClient(TEST_UDP_ENDPOINT_1, tcpConn);

    ASSERT_EQ(server->getClientIDs().size(), 1);
    ASSERT_EQ(server->getClientIDs()[0], tcpConn->getID());
}

TEST_F(NetworkServerTest, MultipleClientsCanConnect)
{
    std::shared_ptr<MockTCPConnection> tcpConn1;
    std::shared_ptr<MockTCPConnection> tcpConn2;

    connectClient(TEST_UDP_ENDPOINT_1, tcpConn1);
    connectClient(TEST_UDP_ENDPOINT_2, tcpConn2);

    const auto& ids = server->getClientIDs();

    ASSERT_EQ(ids.size(), 2u);

    EXPECT_NE(std::find(ids.begin(), ids.end(), tcpConn1->getID()), ids.end());
    EXPECT_NE(std::find(ids.begin(), ids.end(), tcpConn2->getID()), ids.end());
}

TEST_F(NetworkServerTest, CanReceiveTCPMessage)
{
    std::shared_ptr<MockTCPConnection> tcpConn;
    connectClient(TEST_UDP_ENDPOINT_1, tcpConn);

    Message appMsg{ .type = MessageType::PlayerJoined };
    tcpConn->received.push(toTCPMessage(appMsg));

    auto messages = server->consumeIncomingMessages(tcpConn->getID());
    ASSERT_EQ(messages.size(), 0u);

    server->pumpReceive();

    messages = server->consumeIncomingMessages(tcpConn->getID());
    ASSERT_EQ(messages.size(), 1u);
    EXPECT_EQ(messages[0].type, MessageType::PlayerJoined);
}

TEST_F(NetworkServerTest, CanReceiveUDPMessage)
{
    std::shared_ptr<MockTCPConnection> tcpConn;
    connectClient(TEST_UDP_ENDPOINT_1, tcpConn);

    Message appMsg{ .type = MessageType::PlayerJoined };
    udpTransport->received.push({ UDPMessage{appMsg}, TEST_UDP_ENDPOINT_1 });

    auto messages = server->consumeIncomingMessages(tcpConn->getID());
    ASSERT_EQ(messages.size(), 0u);

    server->pumpReceive();

    messages = server->consumeIncomingMessages(tcpConn->getID());
    ASSERT_EQ(messages.size(), 1u);
    EXPECT_EQ(messages[0].type, MessageType::PlayerJoined);
}

TEST_F(NetworkServerTest, CanSendTCPMessage)
{
    std::shared_ptr<MockTCPConnection> tcpConn;
    connectClient(TEST_UDP_ENDPOINT_1, tcpConn);

    Message msg{ .type = MessageType::PlayerLeft };

    ASSERT_EQ(tcpConn->sent.size(), 0u);
    server->queueOutgoingMessage(tcpConn->getID(), msg, constants::TransportType::TCP);
    ASSERT_EQ(tcpConn->sent.size(), 0u);

    server->pumpSend();

    ASSERT_EQ(tcpConn->sent.size(), 1u);
    ASSERT_EQ(fromTCPMessage(tcpConn->sent[0]).type, MessageType::PlayerLeft);
}

TEST_F(NetworkServerTest, CanSendUDPMessage)
{
    std::shared_ptr<MockTCPConnection> tcpConn;
    connectClient(TEST_UDP_ENDPOINT_1, tcpConn);

    Message msg{ .type = MessageType::PlayerLeft };

    ASSERT_EQ(udpTransport->sent.size(), 0u);
    server->queueOutgoingMessage(tcpConn->getID(), msg, constants::TransportType::UDP);
    ASSERT_EQ(udpTransport->sent.size(), 0u);

    server->pumpSend();

    ASSERT_EQ(udpTransport->sent.size(), 1u);

    const auto& [actualMsg, actualRecipient] = udpTransport->sent[0];
    ASSERT_EQ(fromUDPMessage(actualMsg).type, MessageType::PlayerLeft);
    ASSERT_EQ(actualRecipient, TEST_UDP_ENDPOINT_1);
}

TEST_F(NetworkServerTest, TCPDisconnectDisconnectsClient)
{
    std::shared_ptr<MockTCPConnection> tcpConn;
    connectClient(TEST_UDP_ENDPOINT_1, tcpConn);

    ASSERT_EQ(server->getClientIDs().size(), 1u);

    tcpConn->open = false; // simulate disconnect

    server->cleanupDisconnectedClients();
    EXPECT_TRUE(server->getClientIDs().empty());
}

TEST_F(NetworkServerTest, UDPTransportDisconnectDisconnectsAllClients)
{
    std::shared_ptr<MockTCPConnection> tcpConn1, tcpConn2;
    connectClient(TEST_UDP_ENDPOINT_1, tcpConn1);
    connectClient(TEST_UDP_ENDPOINT_2, tcpConn2);

    ASSERT_EQ(server->getClientIDs().size(), 2u);

    udpTransport->open = false; // simulate disconnect

    server->cleanupDisconnectedClients();
    EXPECT_TRUE(server->getClientIDs().empty());
}

TEST_F(NetworkServerTest, IgnoresUDPMessageFromUnknownSender)
{
    Message msg{ .type = MessageType::PlayerJoined };
    udpTransport->received.push({ UDPMessage{msg}, TEST_UDP_ENDPOINT_1 });

    server->pumpReceive();

    EXPECT_TRUE(server->getClientIDs().empty());
}

TEST_F(NetworkServerTest, QueueMessageForNonExistentClient)
{
    Message msg{ .type = MessageType::PlayerLeft };

    // Should not crash or send
    server->queueOutgoingMessage(9999, msg, constants::TransportType::TCP);
    server->queueOutgoingMessage(9999, msg, constants::TransportType::UDP);

    server->pumpSend();

    EXPECT_TRUE(udpTransport->sent.empty());
    EXPECT_TRUE(server->getClientIDs().empty());
}

TEST_F(NetworkServerTest, ClientConnectCallbackFires)
{
    bool connectFired = false;
    server->setOnClientConnect([&](ClientID){ connectFired = true; });

    std::shared_ptr<MockTCPConnection> tcpConn;
    connectClient(TEST_UDP_ENDPOINT_1, tcpConn);

    EXPECT_TRUE(connectFired);
}

TEST_F(NetworkServerTest, ClientDisconnectCallbackFires)
{
    bool disconnectFired = false;
    server->setOnClientDisconnect([&](ClientID){ disconnectFired = true; });

    std::shared_ptr<MockTCPConnection> tcpConn;
    connectClient(TEST_UDP_ENDPOINT_1, tcpConn);

    EXPECT_FALSE(disconnectFired);

    tcpConn->open = false; // simulate disconnect
    server->cleanupDisconnectedClients();

    EXPECT_TRUE(disconnectFired);
}

TEST_F(NetworkServerTest, UDPMessageBeforeBindIsIgnored)
{
    // Create TCP connection but do not complete UDP bind
    auto tcpConn = std::make_shared<MockTCPConnection>();
    tcpListener->nextConnections.push(tcpConn);
    server->acceptConnections();
    server->pumpSend(); // UDPBindRequest sent

    // Send UDP message from endpoint before binding
    Message msg{ .type = MessageType::PlayerJoined };
    udpTransport->received.push({ UDPMessage{msg}, TEST_UDP_ENDPOINT_1 });

    server->pumpReceive();

    // Should not be delivered
    EXPECT_TRUE(server->consumeIncomingMessages(tcpConn->getID()).empty());
}

TEST_F(NetworkServerTest, TCPMessagesDeliverToCorrectClient)
{
    std::shared_ptr<MockTCPConnection> tcpConn1, tcpConn2;
    connectClient(TEST_UDP_ENDPOINT_1, tcpConn1);
    connectClient(TEST_UDP_ENDPOINT_2, tcpConn2);

    // Each client sends a TCP message
    Message msg1{ .type = MessageType::PlayerJoined };
    Message msg2{ .type = MessageType::PlayerLeft };

    tcpConn1->received.push(toTCPMessage(msg1));
    tcpConn2->received.push(toTCPMessage(msg2));

    server->pumpReceive();

    auto inbox1 = server->consumeIncomingMessages(tcpConn1->getID());
    auto inbox2 = server->consumeIncomingMessages(tcpConn2->getID());

    ASSERT_EQ(inbox1.size(), 1u);
    EXPECT_EQ(inbox1[0].type, MessageType::PlayerJoined);

    ASSERT_EQ(inbox2.size(), 1u);
    EXPECT_EQ(inbox2[0].type, MessageType::PlayerLeft);
}

TEST_F(NetworkServerTest, UDPMessagesDeliverToCorrectClient)
{
    std::shared_ptr<MockTCPConnection> tcpConn1, tcpConn2;
    connectClient(TEST_UDP_ENDPOINT_1, tcpConn1);
    connectClient(TEST_UDP_ENDPOINT_2, tcpConn2);

    Message msg1{ .type = MessageType::PlayerJoined };
    Message msg2{ .type = MessageType::PlayerLeft };

    udpTransport->received.push({ UDPMessage{msg1}, TEST_UDP_ENDPOINT_1 });
    udpTransport->received.push({ UDPMessage{msg2}, TEST_UDP_ENDPOINT_2 });

    server->pumpReceive();

    auto inbox1 = server->consumeIncomingMessages(tcpConn1->getID());
    auto inbox2 = server->consumeIncomingMessages(tcpConn2->getID());

    ASSERT_EQ(inbox1.size(), 1u);
    EXPECT_EQ(inbox1[0].type, MessageType::PlayerJoined);

    ASSERT_EQ(inbox2.size(), 1u);
    EXPECT_EQ(inbox2[0].type, MessageType::PlayerLeft);
}

TEST_F(NetworkServerTest, PreservesMessageQueueOrder)
{
    std::shared_ptr<MockTCPConnection> tcpConn;
    connectClient(TEST_UDP_ENDPOINT_1, tcpConn);

    Message msg1{ .type = MessageType::PlayerJoined };
    Message msg2{ .type = MessageType::PlayerLeft };

    server->queueOutgoingMessage(tcpConn->getID(), msg1, constants::TransportType::TCP);
    server->queueOutgoingMessage(tcpConn->getID(), msg2, constants::TransportType::TCP);

    ASSERT_EQ(tcpConn->sent.size(), 0u);

    server->pumpSend();

    ASSERT_EQ(tcpConn->sent.size(), 2u);
    EXPECT_EQ(fromTCPMessage(tcpConn->sent[0]).type, MessageType::PlayerJoined);
    EXPECT_EQ(fromTCPMessage(tcpConn->sent[1]).type, MessageType::PlayerLeft);
}

TEST_F(NetworkServerTest, UDPBindFailsWithInvalidToken)
{
    // Create TCP connection but do not complete UDP bind yet
    auto tcpConn = std::make_shared<MockTCPConnection>();
    tcpListener->nextConnections.push(tcpConn);
    server->acceptConnections();
    server->pumpSend(); // UDPBindRequest sent

    Message invalidBind{
        .type = MessageType::UDPBind,
        .data = { .udpBind = { .token = 99999 } } // invalid token
    };

    udpTransport->received.push({ UDPMessage{invalidBind}, TEST_UDP_ENDPOINT_1 });
    server->pumpReceive();

    // Client should still be in BindingUDP state (not connected)
    server->queueOutgoingMessage(tcpConn->getID(),
                                 Message{.type = MessageType::PlayerJoined},
                                 constants::TransportType::UDP);
    server->pumpSend();

    // No UDP message should be sent
    EXPECT_TRUE(udpTransport->sent.empty());
}

TEST_F(NetworkServerTest, RepeatedConnectDisconnect)
{
    for (int i = 0; i < 5; ++i)
    {
        std::shared_ptr<MockTCPConnection> tcpConn;
        asio::ip::udp::endpoint endpoint(asio::ip::make_address("127.0.0.1"), 40000 + i);

        connectClient(endpoint, tcpConn);
        EXPECT_EQ(server->getClientIDs().size(), 1);

        tcpConn->open = false; // simulate disconnect
        server->cleanupDisconnectedClients();
        EXPECT_TRUE(server->getClientIDs().empty());
    }
}
