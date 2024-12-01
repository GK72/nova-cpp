/**
 * Part of Nova C++ Library.
 *
 * TCP server.
 */

#pragma once

#include <nova/data.hh>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/detail/error_code.hpp>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <utility>

namespace nova::tcp {

using port_type = std::uint_least16_t;

struct net_config {
    port_type port;
};

struct connection_info {
    std::string address;
    std::uint_least16_t port;
};

class handler {
public:
    virtual auto process(nova::data_view) -> std::size_t = 0;
    virtual void on_connection_init(const connection_info&) = 0;
    virtual void on_error(const boost::system::error_code&, const connection_info&) = 0;
    virtual void on_error(const std::exception&, const connection_info&) = 0;
    virtual ~handler() = default;
};

class handler_factory {
public:
    virtual auto create() -> std::unique_ptr<handler> = 0;
    virtual ~handler_factory() = default;
};

class server {
public:
    server(const net_config& cfg);

    /**
     * @brief   Start the TCP server.
     *
     * It's a blocking call.
     */
    void start();

    void stop();

    void set(std::unique_ptr<handler_factory> factory) {
        m_factory = std::move(factory);
    }

    [[nodiscard]] auto port() const -> port_type { return m_config.port; }

private:
    boost::asio::io_context m_io_context;
    boost::asio::ip::tcp::acceptor m_acceptor;
    std::unique_ptr<handler_factory> m_factory;
    net_config m_config;

    auto accept() -> boost::asio::awaitable<void>;
};

} // namespace nova::tcp
