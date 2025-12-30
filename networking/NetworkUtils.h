#include <asio.hpp>
#include <thread>

#include "../common/Logging.h"

#include "tcp/TCPConnection.h"

class IOContextRunner
{
    public:
        IOContextRunner(asio::io_context& ctx)
        : m_ctx(ctx)
        , m_work(asio::make_work_guard(ctx))
        {
            m_thread = std::thread(
                [this]()
                {
                    try
                    {
                        LOG_INFO("ASIO context starting");
                        m_ctx.run();
                        LOG_INFO("ASIO context stopped");
                    }
                    catch (const std::exception& e)
                    {
                        LOG_ERROR("ASIO context exception: %s", e.what());
                        std::terminate();
                    }
                }
            );
        }

        IOContextRunner(const IOContextRunner&) = delete;
        IOContextRunner& operator=(const IOContextRunner&) = delete;
        IOContextRunner(IOContextRunner&&) = delete;
        IOContextRunner& operator=(IOContextRunner&&) = delete;

        ~IOContextRunner()
        {
            m_work.reset();
            m_ctx.stop();
            if (m_thread.joinable())
            {
                LOG_INFO("Joining ASIO context thread");
                m_thread.join();
            }
        }
    private:
        asio::io_context& m_ctx;
        asio::executor_work_guard<asio::io_context::executor_type> m_work;
        std::thread m_thread;
};


inline std::shared_ptr<TCPConnection>
establishTCPConnection(asio::io_context& ioContext,
                       asio::ip::tcp::endpoint endpoint)
{
    LOG_INFO("Establishing TCP connection");

    asio::ip::tcp::socket socket(ioContext);

    std::error_code ec;
    socket.connect(endpoint, ec);

    if (ec)
    {
        throw std::runtime_error(
            "Failed to establish TCP connection: " + ec.message()
        );
    }

    LOG_INFO("Established TCP connection");

    auto tcpConn = TCPConnection::create(ioContext, std::move(socket));
    tcpConn->start();
    return tcpConn;
}
