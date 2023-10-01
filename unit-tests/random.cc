#include "nova/random.h"

#include <gmock/gmock.h>

#include <cmath>

TEST(Random, OneNumber) {
    static constexpr int Number = 1;
    const auto number = nova::random().number(nova::range<int>{ Number, Number });
    EXPECT_EQ(number, Number);
}

TEST(Random, RandomIntInRange) {
    static constexpr int Min = 1;
    static constexpr int Max = 6;

    const auto number = nova::random().number(nova::range<int>{ Min, Max });

    EXPECT_GE(number, Min);
    EXPECT_LE(number, Max);
}

TEST(Random, RandomFloatInRange) {
    static constexpr float Min = 1.0F;
    static constexpr float Max = 6.0F;

    const auto number = nova::random().number(nova::range<float>{ Min, Max });

    EXPECT_GE(number, Min);
    EXPECT_LE(number, Max);
}

TEST(Random, Choice) {
    auto random = nova::rng(1);
    const auto xs = std::to_array({ 3, 6, 9, 12, 15 });

    EXPECT_EQ(random.choice(xs), 9);
}
