#include <gtest/gtest.h>

#include "nova/utils.h"

#include <array>
#include <chrono>
#include <type_traits>

TEST(Utils, Now) {
    using T = decltype(nova::now());
    static_assert(std::is_same_v<T, std::chrono::nanoseconds>);
}

TEST(Utils, ToMicrosec) {
    using namespace std::chrono_literals;
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
