#include <gtest/gtest.h>

#include "game/network_client.h"
#include "game/game_client.h"
#include "game/game_server.h"

#include "utils.h"


class NetworkClientTest : public ::testing::Test
{
protected:
    const asio::ip::udp::endpoint SERVER_UDP_ENDPOINT{
        asio::ip::make_address("127.0.0.1"), 45000
    };

    void SetUp() override
    {
        tcpConn = std::make_shared<MockTCPConnection>();
        udpTransport = std::make_shared<MockUDPTransport>();

        NetworkClient::Config cfg;
        cfg.serverUDPEndpoint = SERVER_UDP_ENDPOINT;
        cfg.udpBindRequestTimeoutMs = 10;
        cfg.udpBindRequestPollMs    = 1;
        cfg.udpBindAckTimeoutMs     = 10;
        cfg.udpBindAckPollMs        = 1;
        // Disable ping as it sends messages, which interfere with tests
        // checking messages.
        cfg.enablePing = false;

        client = std::make_unique<NetworkClient>(tcpConn, udpTransport, cfg);
    }

    void doSuccessfulHandshake()
    {
        Message bindReq{
            .type = MessageType::UDPBindRequest,
            .data = { .udpBindRequest = { .token = 1234 } }
        };
        tcpConn->received.push(toTCPMessage(bindReq));

        std::thread server([&] {
            ASSERT_TRUE(waitFor(
                [&] { return !udpTransport->sent.empty(); },
                200
            )) << "Client never sent UDPBind";

            Message bindAck{
                .type = MessageType::UDPBindAck,
                .data = { .udpBindAck = {} }
            };

            tcpConn->received.push(toTCPMessage(bindAck));
        });

        ASSERT_TRUE(client->doUDPHandshake());
        server.join();

        tcpConn->sent.clear();
        udpTransport->sent.clear();
    }

    std::shared_ptr<MockTCPConnection> tcpConn;
    std::shared_ptr<MockUDPTransport> udpTransport;
    std::unique_ptr<NetworkClient> client;
};

TEST_F(NetworkClientTest, ConnectCompletesUDPHandshake)
{
    ASSERT_FALSE(client->isConnected());
    doSuccessfulHandshake();
    EXPECT_TRUE(client->isConnected());
}

TEST_F(NetworkClientTest, CanReceiveTCPMessages)
{
    doSuccessfulHandshake();

    Message msg{ .type = MessageType::PlayerJoined };

    tcpConn->received.push(toTCPMessage(msg));

    client->pumpReceive();
    auto messages = client->consumeIncomingMessages();

    ASSERT_EQ(messages.size(), 1u);
    EXPECT_EQ(messages[0].type, MessageType::PlayerJoined);
}

TEST_F(NetworkClientTest, CanReceiveUDPMessages)
{
    doSuccessfulHandshake();

    Message msg{ .type = MessageType::PlayerLeft };

    udpTransport->received.push({ toUDPMessage(msg), SERVER_UDP_ENDPOINT });

    client->pumpReceive();
    auto messages = client->consumeIncomingMessages();

    ASSERT_EQ(messages.size(), 1u);
    EXPECT_EQ(messages[0].type, MessageType::PlayerLeft);
}

TEST_F(NetworkClientTest, CanSendTCPMessage)
{
    doSuccessfulHandshake();

    Message tcpMsg{ .type = MessageType::PlayerJoined };

    client->queueOutgoingMessage(tcpMsg, constants::TransportType::TCP);

    client->pumpSend();

    ASSERT_EQ(tcpConn->sent.size(), 1u);
    EXPECT_EQ(fromTCPMessage(tcpConn->sent[0]).type, MessageType::PlayerJoined);
}

TEST_F(NetworkClientTest, CanSendUDPMessage)
{
    doSuccessfulHandshake();

    Message udpMsg{ .type = MessageType::PlayerLeft };

    client->queueOutgoingMessage(udpMsg, constants::TransportType::UDP);

    client->pumpSend();

    ASSERT_EQ(udpTransport->sent.size(), 1u);
    EXPECT_EQ(fromUDPMessage(udpTransport->sent[0].first).type,
              MessageType::PlayerLeft);
    EXPECT_EQ(udpTransport->sent[0].second, SERVER_UDP_ENDPOINT);
}

TEST_F(NetworkClientTest, ConnectFailsIfUDPHandshakeTimesOut)
{
    ASSERT_FALSE(client->isConnected());

    // Server doesn't send any UDPBindRequest or UDPBindAck
    bool success = client->doUDPHandshake();

    EXPECT_FALSE(success);
    EXPECT_FALSE(client->isConnected());
}

TEST_F(NetworkClientTest, IgnoresUDPMessagesFromUnknownEndpoints)
{
    doSuccessfulHandshake();

    Message msg{ .type = MessageType::PlayerLeft };
    asio::ip::udp::endpoint unknownEndpoint{
        asio::ip::make_address("127.0.0.1"), 9999
    };

    udpTransport->received.push({ toUDPMessage(msg), unknownEndpoint });

    client->pumpReceive();
    auto messages = client->consumeIncomingMessages();

    EXPECT_TRUE(messages.empty());
}

TEST_F(NetworkClientTest, ReceivesMessagesFromTCPAndUDP)
{
    doSuccessfulHandshake();

    Message tcpMsg{ .type = MessageType::PlayerJoined };
    Message udpMsg{ .type = MessageType::PlayerLeft };

    tcpConn->received.push(toTCPMessage(tcpMsg));
    udpTransport->received.push({ toUDPMessage(udpMsg), SERVER_UDP_ENDPOINT });

    client->pumpReceive();
    auto messages = client->consumeIncomingMessages();

    ASSERT_EQ(messages.size(), 2u);
    EXPECT_EQ(messages[0].type, MessageType::PlayerJoined);
    EXPECT_EQ(messages[1].type, MessageType::PlayerLeft);
}

TEST_F(NetworkClientTest, PreservesMessageQueueOrder)
{
    doSuccessfulHandshake();

    Message tcpMsg1{ .type = MessageType::PlayerJoined };
    Message tcpMsg2{ .type = MessageType::PlayerLeft };

    Message udpMsg1{ .type = MessageType::PlayerJoined };
    Message udpMsg2{ .type = MessageType::PlayerLeft };

    client->queueOutgoingMessage(tcpMsg1, constants::TransportType::TCP);
    client->queueOutgoingMessage(tcpMsg2, constants::TransportType::TCP);
    client->queueOutgoingMessage(udpMsg1, constants::TransportType::UDP);
    client->queueOutgoingMessage(udpMsg2, constants::TransportType::UDP);

    client->pumpSend();

    ASSERT_EQ(tcpConn->sent.size(), 2u);
    EXPECT_EQ(fromTCPMessage(tcpConn->sent[0]).type, MessageType::PlayerJoined);
    EXPECT_EQ(fromTCPMessage(tcpConn->sent[1]).type, MessageType::PlayerLeft);

    ASSERT_EQ(udpTransport->sent.size(), 2u);
    EXPECT_EQ(fromUDPMessage(udpTransport->sent[0].first).type, MessageType::PlayerJoined);
    EXPECT_EQ(fromUDPMessage(udpTransport->sent[1].first).type, MessageType::PlayerLeft);
}
