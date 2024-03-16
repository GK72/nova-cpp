#include <gtest/gtest.h>

#include "nova/utils.h"

#include <array>
#include <chrono>
#include <thread>
#include <type_traits>

using namespace std::chrono_literals;

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

TEST(Utils, Stopwatch_Elapsed) {
    auto stopwatch = nova::stopwatch();
    EXPECT_GT(stopwatch.elapsed(), 0ns);

    std::this_thread::sleep_for(200ms);
    EXPECT_GT(stopwatch.elapsed(), 200ms);

    std::this_thread::sleep_for(200ms);
    EXPECT_GT(stopwatch.elapsed(), 400ms);
}

TEST(Utils, Stopwatch_Lap) {
    auto stopwatch = nova::stopwatch();
    EXPECT_GT(stopwatch.lap(), 0ns);

    std::this_thread::sleep_for(200ms);
    EXPECT_GT(stopwatch.lap(), 200ms);
    EXPECT_LT(stopwatch.lap(), 100us);
}
