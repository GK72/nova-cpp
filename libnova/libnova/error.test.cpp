#include <libnova/error.hpp>
#include <libnova/expected.hpp>

#include <fmt/format.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <array>
#include <string>
#include <string_view>
#include <system_error>

/**
 * A library outside nova with its own error origin: an enum and a trait.
 */
namespace net {

    enum class error {
        timeout = 1,
        refused
    };

    // Optional: plain `std::error_code` conversion, e.g. `err.code == net_error::timeout`
    using nova::make_error_code;

} // namespace net

template <>
struct nova::error_enum<net::error> {
    static constexpr auto name = "net";
    static constexpr auto messages = std::to_array<std::string_view>({ "timed out", "connection refused" });

    // Optional: checks for the kind of failure without knowing `net`
    static constexpr auto conditions = std::to_array({ std::errc::timed_out, std::errc::connection_refused });
};

namespace net {

    [[nodiscard]] inline
    auto connect(const std::string& host) -> nova::expected<int, nova::error> {
        return nova::wrap_error(error::timeout, host);
    }

} // namespace net

TEST(Error, Error_FmtString) {
    const auto err = nova::error{
        .code = std::make_error_code(std::errc::no_such_file_or_directory),
        .detail = "/etc/missing"
    };

    EXPECT_EQ(fmt::format("{}", err), "[generic] No such file or directory (/etc/missing)");
}

TEST(Error, Error_FmtString_NoDetail) {
    const auto err = nova::error{ .code = std::make_error_code(std::errc::is_a_directory) };
    EXPECT_EQ(fmt::format("{}", err), "[generic] Is a directory");
}

TEST(Error, Error_OtherLibraryCategory) {
    const auto result = net::connect("example.com:80");
    ASSERT_FALSE(result.has_value());

    const auto& err = result.error();
    EXPECT_EQ(fmt::format("{}", err), "[net] timed out (example.com:80)");

    // The exact code, from this origin only
    EXPECT_EQ(err.code, net::error::timeout);
    EXPECT_NE(err.code, std::make_error_code(std::errc::timed_out));

    // The kind of failure, whatever the origin
    EXPECT_EQ(err.code, std::errc::timed_out);
}

TEST(Error, Error_UnknownValue) {
    const auto code = nova::make_error_code(static_cast<net::error>(7));
    EXPECT_EQ(code.message(), "unknown net error 7");
}
