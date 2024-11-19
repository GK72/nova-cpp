#include "nova/expected.h"

#include <gtest/gtest.h>

#include <memory>
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

        EXPECT_TRUE(not ret);
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

TEST(Expected, NonTrivialDestructor_LeakSanyiOn) {
    const auto x = nova::expected<std::unique_ptr<int>, int>(std::make_unique<int>(9));
    EXPECT_TRUE(x);
    // Leak sanitizer fires if the destructor of the implementation (`vex`) is empty.
}

TEST(Expected, SameTypes) {
    constexpr auto x = nova::expected<int, int>(9);
    ASSERT_TRUE(x);
    EXPECT_EQ(*x, 9);

    constexpr auto y = nova::expected<int, int>(nova::unexpected{ 8 });
    ASSERT_TRUE(not y);
    EXPECT_EQ(y.error(), 8);
}

TEST(Expected, ValueConversion) {
    constexpr auto x = nova::expected<std::string_view, int>("hello");
    EXPECT_EQ(x.value(), "hello");
}

TEST(Expected, ErrorConversion) {
    constexpr auto x = nova::expected<int, std::string_view>(nova::unexpected{ "hello" });
    EXPECT_EQ(x.error(), "hello");
}

TEST(Expected, Deref) {
    constexpr auto x = nova::expected<int, std::string_view>(9);
    EXPECT_EQ(*x, 9);
}

TEST(Expected, HasValue) {
    constexpr auto x = nova::expected<int, std::string_view>(1);
    EXPECT_TRUE(x.has_value());

    constexpr auto y = nova::expected<int, std::string_view>(nova::unexpected{ "a" });
    EXPECT_TRUE(not y.has_value());
}

TEST(Expected, BoolConversion) {
    constexpr auto x = nova::expected<int, std::string_view>(1);
    EXPECT_TRUE(x);

    constexpr auto y = nova::expected<int, std::string_view>(nova::unexpected{ "a" });
    EXPECT_TRUE(not y);
}

TEST(Expected_MondadicOps, ValueOr) {
    constexpr auto x = nova::expected<int, std::string_view>(9);
    EXPECT_EQ(x.value_or(2), 9);

    constexpr auto y = nova::expected<int, std::string_view>(nova::unexpected{ "a" });
    EXPECT_EQ(y.value_or(2), 2);
}

TEST(Expected_MondadicOps, ValueOr_OnRValue) {
    EXPECT_EQ(( nova::expected<int, std::string_view>(9).value_or(2) ), 9);
    EXPECT_EQ(( nova::expected<int, std::string_view>(nova::unexpected{ "a" }).value_or(2) ), 2);
}
