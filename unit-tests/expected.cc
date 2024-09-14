#include <string_view>

#include <gtest/gtest.h>

#include "nova/expected.h"

TEST(Expected, TrivialTypes) {
    constexpr auto expect = [](bool condition) -> nova::expected<int, std::string_view> {
        if (condition) {
            return 1;
        }
        return nova::unexpected<error>{"Error message"};
    };

    {
        constexpr auto ret = expect(true);

        EXPECT_TRUE(ret);
        EXPECT_EQ(ret.value(), 1);
        EXPECT_EQ(*ret, 1);
    }

    {
        constexpr auto ret = expect(false);

        using namespace std::literals::string_view_literals;

        EXPECT_TRUE(!ret);
        EXPECT_EQ(ret.error(), "Error message"sv);
    }
}

TEST(Expected, NonTrivialTypes) {
    struct S {
        constexpr S() { }
        constexpr ~S() { }
    };

    constexpr auto x = nova::expected<S, int>(S{ });
    EXPECT_TRUE(x);
}
