#include <gtest/gtest.h>

#include "nova/io.h"

#include <sstream>

TEST(IO, LineParser) {
    std::stringstream ss;
    ss << "Hello\nIO";

    auto parser = nova::line_parser();
    const auto xs = parser(ss);
    ASSERT_EQ(xs.size(), 2);
    EXPECT_EQ(xs[0], "Hello");
    EXPECT_EQ(xs[1], "IO");
}
