#include <gtest/gtest.h>
#include <asio.hpp>

#include "networking/tcp/TCPConnection.h"
#include "networking/tcp/TCPListener.h"
#include "networking/NetworkUtils.h"

#include "TestUtils.h"


// Simple payload used in tests
struct TestPayload
{
    uint32_t value;
};


class TCPConnectionTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        ioContextRunner = std::make_unique<IOContextRunner>(ioContext);

        tcpListener = std::make_shared<TCPListener>(ioContext, 0);
        tcpListener->start();

        tcpListenerPort = tcpListener->getPort();

        clientConn = establishTCPConnection(
            ioContext,
            asio::ip::tcp::endpoint(asio::ip::address_v4::loopback(), tcpListenerPort)
        );

        ASSERT_TRUE(waitFor([&]() {
            serverConn = tcpListener->getNextConnection();
            return serverConn != nullptr;
        }, 200));

        // Setting IDs makes debugging easier because logs mention the ID
        clientConn->setID(clientConnID);
        serverConn->setID(serverConnID);
    }

    asio::io_context ioContext;
    std::unique_ptr<IOContextRunner> ioContextRunner;

    std::shared_ptr<TCPListener> tcpListener;
    uint16_t tcpListenerPort;

    std::shared_ptr<TCPConnection> clientConn;
    std::shared_ptr<TCPConnectionInterface> serverConn;

    u_int32_t clientConnID = 123;
    u_int32_t serverConnID = 0;
};


TEST_F(TCPConnectionTest, ConnectionsStartOpen)
{
    EXPECT_TRUE(clientConn->isOpen());
    EXPECT_TRUE(serverConn->isOpen());
}

TEST_F(TCPConnectionTest, ClientCanSendAndServerReceives)
{
    TCPMessage msg(TCPMessageType::Data, TestPayload{42});

    clientConn->send(msg);

    TCPMessage receivedMsg;
    ASSERT_TRUE(waitFor([&]() {
        return serverConn->tryRecv(receivedMsg);
    }, 200));

    EXPECT_EQ(receivedMsg.header.type, TCPMessageType::Data);
    EXPECT_EQ(receivedMsg.body<TestPayload>().value, 42u);
}

TEST_F(TCPConnectionTest, ServerCanSendAndClientReceives)
{
    TCPMessage msg(TCPMessageType::Data, TestPayload{99});

    serverConn->send(msg);

    TCPMessage receivedMsg;
    ASSERT_TRUE(waitFor([&]() {
        return clientConn->tryRecv(receivedMsg);
    }, 200));

    EXPECT_EQ(receivedMsg.header.type, TCPMessageType::Data);
    EXPECT_EQ(receivedMsg.body<TestPayload>().value, 99u);
}

TEST_F(TCPConnectionTest, MultipleMessagesAreDeliveredInOrder)
{
    constexpr uint32_t numMsgs = 10;
    for (uint32_t i = 0; i < numMsgs; ++i)
    {
        clientConn->send(TCPMessage(TCPMessageType::Data, TestPayload{i}));
    }

    for (uint32_t i = 0; i < numMsgs; ++i)
    {
        TCPMessage receivedMsg;
        ASSERT_TRUE(waitFor([&]() {
            return serverConn->tryRecv(receivedMsg);
        }, 200));
        EXPECT_EQ(receivedMsg.body<TestPayload>().value, i);
    }
}

TEST_F(TCPConnectionTest, DisconnectClosesConnection)
{
    clientConn->disconnect();
    EXPECT_FALSE(clientConn->isOpen());
}

TEST_F(TCPConnectionTest, SendOnClosedConnectionIsNoOp)
{
    clientConn->disconnect();
    EXPECT_FALSE(clientConn->isOpen());

    clientConn->send(TCPMessage(TCPMessageType::Data, TestPayload{123}));

    TCPMessage receivedMsg;
    ASSERT_FALSE(waitFor([&]() {
        return serverConn->tryRecv(receivedMsg);
    }, 200));
}

TEST_F(TCPConnectionTest, ClientClosedClosesServer)
{
    ASSERT_TRUE(clientConn->isOpen());
    ASSERT_TRUE(serverConn->isOpen());

    clientConn->disconnect();

    EXPECT_TRUE(waitFor([&]() {
        return !serverConn->isOpen();
    }, 2000));

    EXPECT_TRUE(waitFor([&]() {
        return !clientConn->isOpen();
    }, 2000));
}

TEST_F(TCPConnectionTest, ServerClosedClosesClient)
{
    ASSERT_TRUE(clientConn->isOpen());
    ASSERT_TRUE(serverConn->isOpen());

    serverConn->disconnect();

    EXPECT_TRUE(waitFor([&]() {
        return !serverConn->isOpen();
    }, 2000));

    EXPECT_TRUE(waitFor([&]() {
        return !clientConn->isOpen();
    }, 2000));
}
