#include <gtest/gtest.h>
#include <asio.hpp>

#include "../networking/udp/UDPTransport.h"
#include "../networking/NetworkUtils.h"

#include "TestUtils.h"


// Simple payload used in tests
struct TestPayload
{
    uint32_t value;
};


class UDPTransportTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Note, starts the runner immediately
        ioContextRunner = std::make_unique<IOContextRunner>(ioContext);

        testSender = UDPTransport::create(ioContext);
        testReceiver = UDPTransport::create(ioContext);

        testSender->openEphemeral();
        testReceiver->openEphemeral();

        testReceiverEndpoint = asio::ip::udp::endpoint(
            asio::ip::address_v4::loopback(),
            testReceiver->getLocalPort()
        );

        testSenderEndpoint = asio::ip::udp::endpoint(
            asio::ip::address_v4::loopback(),
            testSender->getLocalPort()
        );
    }

    asio::io_context ioContext;
    std::unique_ptr<IOContextRunner> ioContextRunner;

    std::shared_ptr<UDPTransport> testSender;
    std::shared_ptr<UDPTransport> testReceiver;

    asio::ip::udp::endpoint testSenderEndpoint;
    asio::ip::udp::endpoint testReceiverEndpoint;
};


TEST_F(UDPTransportTest, CanOpenOnEphemeralPort)
{
    auto transport = UDPTransport::create(ioContext);

    EXPECT_FALSE(transport->isOpen());

    transport->openEphemeral();

    EXPECT_TRUE(transport->isOpen());
    EXPECT_GT(transport->getLocalPort(), 0u);
}

TEST_F(UDPTransportTest, CanOpenOnBoundPort)
{
    auto transport = UDPTransport::create(ioContext);

    EXPECT_FALSE(transport->isOpen());

    u_int16_t port = 55000;

    transport->openBound(port);

    EXPECT_TRUE(transport->isOpen());
    EXPECT_EQ(transport->getLocalPort(), port);
}

TEST_F(UDPTransportTest, CanSendAndReceiveMessage)
{
    UDPMessage msg(TestPayload{42});

    testSender->sendTo(msg, testReceiverEndpoint);

    UDPMessage receivedMsg;
    asio::ip::udp::endpoint actualSenderEndpoint;

    ASSERT_TRUE(waitFor([&] {
        return testReceiver->tryRecv(receivedMsg, actualSenderEndpoint);
    }, 200));

    EXPECT_EQ(receivedMsg.data<TestPayload>().value, 42u);
    EXPECT_EQ(actualSenderEndpoint.port(), testSenderEndpoint.port());
    EXPECT_EQ(actualSenderEndpoint.address(), testSenderEndpoint.address());
}

TEST_F(UDPTransportTest, ClosedTransportDoesNotReceiveNewMessages)
{
    UDPMessage msg(TestPayload{123});

    testReceiver->close();
    EXPECT_FALSE(testReceiver->isOpen());

    testSender->sendTo(msg, testReceiverEndpoint);

    UDPMessage receivedMsg;
    asio::ip::udp::endpoint _senderEndpoint;

    ASSERT_FALSE(waitFor([&]() {
        return testReceiver->tryRecv(receivedMsg, _senderEndpoint);
    }, 200));
}

TEST_F(UDPTransportTest, CanSendAndReceiveManyMessages)
{
    constexpr uint32_t numMsgs = 100;

    for (uint32_t i = 0; i < numMsgs; ++i)
    {
        testSender->sendTo(UDPMessage(TestPayload{i}), testReceiverEndpoint);
    }

    std::vector<TestPayload> receivedMsgs;
    for (int i = 0; i < numMsgs; ++i)
    {
        UDPMessage msg;
        asio::ip::udp::endpoint _senderEndpoint;
        if (waitFor([&]() {
            return testReceiver->tryRecv(msg, _senderEndpoint);
        }, 20))
        {
            receivedMsgs.push_back(msg.data<TestPayload>());
        }
    }

    LOG_INFO("Sent %d messages, received %zu", numMsgs, receivedMsgs.size());

    // Expect at least 95% received, because packets may be dropped
    ASSERT_GE(receivedMsgs.size(), numMsgs * 0.95);
}

TEST_F(UDPTransportTest, SendOnClosedTransportIsNoOp)
{
    testSender->close();
    EXPECT_FALSE(testSender->isOpen());

    testSender->sendTo(UDPMessage(TestPayload{999}), testReceiverEndpoint);

    UDPMessage receivedMsg;
    asio::ip::udp::endpoint senderEndpoint;

    EXPECT_FALSE(waitFor([&]()
    {
        return testReceiver->tryRecv(receivedMsg, senderEndpoint);
    }, 100));
}
