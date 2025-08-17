#include <libnova/io.hpp>

#include <gtest/gtest.h>

#include <cstddef>
#include <sstream>

TEST(Io, LineParser) {
    std::stringstream ss;
    ss << "Hello\nIO";

    auto parser = nova::line_parser();
    const auto xs = parser(ss);
    ASSERT_EQ(xs.size(), 2);
    EXPECT_EQ(xs[0], "Hello");
    EXPECT_EQ(xs[1], "IO");
}

TEST(Io, LineParser_CustomCallback) {
    struct custom_parser {
        auto operator()(const std::string& line) const -> std::size_t {
            return line.size();
        }
    };

    std::stringstream ss;
    ss << "Hello\nIO";

    auto parser = nova::line_parser(custom_parser{});
    const auto xs = parser(ss);
    ASSERT_EQ(xs.size(), 2);
    EXPECT_EQ(xs[0], 5);
    EXPECT_EQ(xs[1], 2);
}

TEST(Io, ReadBinary) {
    std::stringstream ss;
    ss << '\x00' << '\x05' << '\x10';

    auto parser = nova::detail::def_bin_parser{};
    const auto xs = parser(ss);
    ASSERT_EQ(xs.size(), 3);
    EXPECT_EQ(xs[0], std::byte{  0 });
    EXPECT_EQ(xs[1], std::byte{  5 });
    EXPECT_EQ(xs[2], std::byte{ 16 });
}
