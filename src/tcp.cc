/**
 * Part of Nova C++ Library.
 *
 * TCP server.
 */

#include "nova/tcp.hh"
#include "nova/units.hh"

#include <boost/asio/buffer.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>

#include <spdlog/spdlog.h>

#include <cstdint>

#include <coroutine>
#include <memory>

namespace asio = boost::asio;
using asio::ip::tcp;

namespace nova::tcp {

// class connection;

/**
 * @brief   Connection proxy
 */
// class conproxy {
// public:
    // conproxy(connection& conn)
        // : m_connection(conn)
    // {}

    // auto send(nova::data_view data) -> asio::awaitable<void>;

// private:
    // connection& m_connection;
// };

class connection : public std::enable_shared_from_this<connection> {
    static constexpr nova::units::bytes BufferSize { nova::units::MBytes{ 1 } };

public:
    explicit connection(::tcp::socket socket, std::unique_ptr<handler> handler)
        : m_socket(std::move(socket))
        , m_handler(std::move(handler))
        , m_connection_info({
            m_socket.remote_endpoint().address().to_string(),
            m_socket.remote_endpoint().port()
        })
    {
        m_handler->on_connection_init(m_connection_info);
    }

    void start() {
        asio::co_spawn(
            m_socket.get_executor(),
            [self = shared_from_this()] { return self->handle_connection(); },
            asio::detached
        );
    }

    void close() {
        m_socket.shutdown(boost::asio::socket_base::shutdown_both);
        m_socket.close();
    }

    auto send(nova::data_view data) -> asio::awaitable<void> {
        co_await asio::async_write(m_socket, asio::buffer(data.ptr(), data.size()), asio::use_awaitable);
    }

private:
    ::tcp::socket m_socket;
    std::unique_ptr<handler> m_handler;

    connection_info m_connection_info;

    /**
     * @brief   Read from socket and call the connection handler.
     *
     * The connection has an internal buffer from which the connection handler
     * is served as long as it can process some of the data.
     *
     * All thrown exceptions are reported back to handler and the connection
     * is closed after that.
     */
    auto handle_connection() -> asio::awaitable<void> {
        auto buf = asio::streambuf{};

        while (true) {
            auto buffer = buf.prepare(BufferSize.count());

            boost::system::error_code ec;
            std::size_t n = co_await m_socket.async_read_some(
                buffer,
                asio::redirect_error(asio::use_awaitable, ec)
            );

            if (ec) {
                m_handler->on_error(ec, m_connection_info);
            }

            if (n == 0) {
                continue;
            }

            buf.commit(n);

            try {
                while (
                    auto processed = m_handler->process(
                        nova::data_view{
                            buf.data().data(),
                            buf.size()
                        }
                    )
                ) {
                    buf.consume(processed);
                }
            } catch (const std::exception& ex) {
                m_handler->on_error(ex, m_connection_info);
                close();
                break;
            } catch (...) {
                m_handler->on_error(std::runtime_error("Unknown error"), m_connection_info);
                close();
                break;
            }
        }
    }
};

server::server(const net_config& cfg)
    : m_acceptor(
        m_io_context,
        ::tcp::endpoint{ ::tcp::v6(), cfg.port }
    )
    , m_config(cfg)
{}

void server::start() {
    asio::co_spawn(
        m_acceptor.get_executor(),
        [this]() { return accept(); },
        asio::detached
    );

    m_io_context.run();
}

void server::stop() {
    m_io_context.stop();
}

/**
 * @brief   Accept an incoming connection and create a connection handler.
 */
auto server::accept() -> asio::awaitable<void> {
    try {
        while (true) {
            ::tcp::socket socket = co_await m_acceptor.async_accept(asio::use_awaitable);
            std::make_shared<connection>(std::move(socket), m_factory->create())->start();
        }
    } catch (const std::exception& e) {
        spdlog::error("{}", e.what());
    }
}

// auto send(nova::data_view data) -> asio::awaitable<void> {
    // co_await asio::async_write(m_connection.m_socket, asio::buffer(data.as_string(0, data.size())), asio::use_awaitable);
// }

} // namespace nova::tcp
