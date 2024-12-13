/**
 * Part of Nova C++ Library.
 *
 * TCP server.
 */

#pragma once

#include "nova/tcp_handler.hh"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnull-dereference"
#include <boost/asio/awaitable.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#pragma GCC diagnostic pop

#include <cstdint>
#include <memory>
#include <utility>

namespace nova::tcp {

using port_type = std::uint_least16_t;

struct net_config {
    std::string host;
    port_type port;
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

class client {
public:
    client();

    void connect(const net_config& cfg);
    void connect(const std::string& address);
    auto send(nova::data_view) -> bytes;

private:
    boost::asio::io_context m_io_context;
    boost::asio::ip::tcp::socket m_socket;

};

} // namespace nova::tcp
