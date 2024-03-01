#include <gtest/gtest.h>

#include "nova/static_string.h"

#include <string>
#include <type_traits>

using namespace nova::literals;
using namespace std::string_literals;

TEST(StaticString, Size) {
    constexpr auto str = nova::static_string{ "hello" };
    static_assert(str.size() == 5);
}

TEST(StaticString, Literal) {
    static_assert("hello"_str .size() == 5);
}

TEST(StaticString, SeparatorDeductionGuide) {
    constexpr auto sep = nova::separator{ "." };
    static_assert(std::is_same_v<decltype(sep.value), nova::static_string<1>>);
}

TEST(StaticString, Comparisons) {
    constexpr auto str  = "Hello"_str;
    constexpr auto str2 = "Hello"_str;
    static_assert(str == str);
    static_assert(str == str2);

    // Generated from spaceship operator
    static_assert("Alice"_str  < "Bob"_str);
    static_assert("Alice"_str <= "Bob"_str);
    static_assert("Bob"_str    > "Alice"_str);
    static_assert("Bob"_str   >= "Alice"_str);

    // Generated from equality operator
    static_assert("Bob"_str   != "Alice"_str);

    // Testing different but equal length strings
    static_assert("-a"_str < "-b"_str);
    static_assert("-b"_str > "-a"_str);
}

TEST(StaticString, Concat) {
    static_assert(nova::concat("Hello", " ", "World") == "Hello World"_str);
    static_assert(nova::concat(nova::separator{ " " }, "Hello", "World") == "Hello World"_str);
    EXPECT_EQ(nova::concat(nova::separator{ " " }, "Hello", "World"s), "Hello World"s);
}
