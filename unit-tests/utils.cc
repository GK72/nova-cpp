#include "nova/types.hh"
#include "nova/utils.hh"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <array>
#include <chrono>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

using namespace std::chrono_literals;

TEST(Utils, Split_EmptyString) {
    EXPECT_EQ(
        nova::split("", "/"),
        ( std::vector<std::string>{ } )
    );
}

TEST(Utils, Split_LeadingDelim) {
    EXPECT_EQ(
        nova::split("/bla", "/"),
        ( std::vector<std::string>{ "", "bla" } )
    );
}

TEST(Utils, Split_LeadingAndTrailingDelim) {
    EXPECT_EQ(
        nova::split("/bla/", "/"),
        ( std::vector<std::string>{ "", "bla", "" } )
    );
}

TEST(Utils, Split_MultiElems) {
    EXPECT_EQ(
        nova::split("bla/abc", "/"),
        ( std::vector<std::string>{ "bla", "abc" } )
    );
}

TEST(Utils, Split_MultiCharDelim) {
    EXPECT_EQ(
        nova::split("bla//abc", "//"),
        ( std::vector<std::string>{ "bla", "abc" } )
    );
}

TEST(Utils, Split_StdString) {
    using namespace std::string_literals;
    EXPECT_EQ(
        nova::split("bla//abc"s, "//"),
        ( std::vector<std::string>{ "bla", "abc" } )
    );
}

TEST(Utils, Now) {
    using T = decltype(nova::now());
    static_assert(std::is_same_v<T, std::chrono::nanoseconds>);
}

TEST(Utils, ToMicrosec) {
    constexpr auto microsec = nova::to_us(9s);
    EXPECT_EQ(microsec, 9'000'000.0);
}

TEST(Utils, Concat) {
    EXPECT_EQ(
        nova::concat(
            std::to_array({ 4, 6, 8 }),
            std::to_array({ 1, 7, 2 }),
            std::to_array({ 2, 3, 1 })
        ),
        std::to_array({
            4, 6, 8,
            1, 7, 2,
            2, 3, 1
        })
    );
}

TEST(Utils, GetEnv) {
    EXPECT_FALSE(nova::getenv("NONEXISTENTENV").has_value());
    EXPECT_EQ(nova::getenv("NOVA_TEST_ENV").value(), "1");
    EXPECT_EQ(nova::getenv("NONEXISTENTENV", "default"), "default");
}

TEST(Utils, Linspace) {
    using namespace testing;

    EXPECT_THAT(
        nova::linspace(nova::range<int>{ 0, 100 }, 10, false),
        ElementsAre(
             FloatEq( 0.0F),
             FloatEq(10.0F),
             FloatEq(20.0F),
             FloatEq(30.0F),
             FloatEq(40.0F),
             FloatEq(50.0F),
             FloatEq(60.0F),
             FloatEq(70.0F),
             FloatEq(80.0F),
             FloatEq(90.0F)
        )
    );

    EXPECT_THAT(
        nova::linspace(nova::range<int>{ 0, 2 }, 5, false),
        ElementsAre(
            FloatEq(0.0F),
            FloatEq(0.4F),
            FloatEq(0.8F),
            FloatEq(1.2F),
            FloatEq(1.6F)
        )
    );

    EXPECT_THAT(
        nova::linspace(nova::range<int>{ -2, 2 }, 4, false),
        ElementsAre(
            FloatEq(-2.0F),
            FloatEq(-1.0F),
            FloatEq( 0.0F),
            FloatEq( 1.0F)
        )
    );

    EXPECT_THAT(
        nova::linspace(nova::range<int>{ -2, 2 }, 5),
        ElementsAre(
            FloatEq(-2.0F),
            FloatEq(-1.0F),
            FloatEq( 0.0F),
            FloatEq( 1.0F),
            FloatEq( 2.0F)
        )
    );
}

TEST(Utils, Linspace_ExplicitReturnType) {
    using namespace testing;

    EXPECT_THAT(
        nova::linspace<double>(nova::range<int>{ -2, 2 }, 5),
        ElementsAre(
            DoubleEq(-2.0),
            DoubleEq(-1.0),
            DoubleEq( 0.0),
            DoubleEq( 1.0),
            DoubleEq( 2.0)
        )
    );
}

TEST(Utils, Stopwatch_Elapsed) {
    auto stopwatch = nova::stopwatch();
    EXPECT_GE(stopwatch.elapsed(), 0ns);

    std::this_thread::sleep_for(200ms);
    EXPECT_GE(stopwatch.elapsed(), 200ms);

    std::this_thread::sleep_for(200ms);
    EXPECT_GE(stopwatch.elapsed(), 400ms);
}

TEST(Utils, Stopwatch_Lap) {
    auto stopwatch = nova::stopwatch();
    EXPECT_GE(stopwatch.lap(), 0ns);

    std::this_thread::sleep_for(200ms);
    EXPECT_GE(stopwatch.lap(), 200ms);
}
