#include "nova/expected.hh"

#include <gtest/gtest.h>

#include <string_view>

TEST(Expected, TrivialTypes) {
    constexpr auto expect = [](bool condition) -> nova::expected<int, std::string_view> {
        if (condition) {
            return 1;
        }
        return nova::unexpected<std::string_view>{"Error message"};
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

    const auto x = nova::expected<S, int>(S{ });
    EXPECT_TRUE(x);
}
