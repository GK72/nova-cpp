#pragma once

#include "nova/data.hh"

#include <boost/system/detail/error_code.hpp>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>

namespace nova::tcp {

using bytes = std::vector<std::uint8_t>;

struct connection_info {
    std::string address;
    std::uint_least16_t port;
};

// tag::tcp-handler[]
class handler {
public:
    virtual auto process(nova::data_view) -> std::size_t = 0;
    virtual void on_connection_init(const connection_info&) = 0;
    virtual void on_error(const boost::system::error_code&, const connection_info&) = 0;
    virtual void on_error(const std::exception&, const connection_info&) = 0;
    virtual ~handler() = default;
};
// end::tcp-handler[]

class handler_factory {
public:
    virtual auto create() -> std::unique_ptr<handler> = 0;
    virtual ~handler_factory() = default;
};

} // namespace nova::tcp
