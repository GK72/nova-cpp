#define NOVA_RUNTIME_ASSERTIONS

#include "nova/error.h"
#include "nova/random.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <array>
#include <ranges>
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

TEST(Random, RandomFloatInDefaultRange) {
    static constexpr double Min = 0.0;
    static constexpr double Max = 1.0;

    const auto number = nova::random().number();

    EXPECT_GE(number, Min);
    EXPECT_LE(number, Max);
}

TEST(Random, Choice) {
    auto random = nova::rng(1);
    constexpr auto xs = std::to_array({ 3, 6, 9, 12, 15 });

    const auto result = random.choice(xs);
    EXPECT_TRUE(std::ranges::find(xs, result) != std::ranges::end(xs));
}

TEST(Random, RandomString) {
    static constexpr std::size_t Length = 10;
    auto random = nova::rng(1);

    // TODO(test-imp): not consistent across platforms, exact match cannot be used
    EXPECT_EQ(
        random.string<nova::ascii_distribution>(Length).length(),
        Length
        // random.string<nova::ascii_distribution>(Length),
        // "G~dx ,<~-6"
    );
}

TEST(Random, RandomString_AlphanumericDistribution) {
    static constexpr std::size_t Length = 10;
    auto random = nova::rng(1);

    // TODO(test-imp): not consistent across platforms, exact match cannot be used
    EXPECT_EQ(
        random.string<nova::alphanumeric_distribution>(Length).length(),
        Length
        // random.string<nova::alphanumeric_distribution>(Length),
        // "z9S5ahs9jo"
    );
}

TEST(Random, RandomString_AlphabeticDistribution) {
    static constexpr std::size_t Length = 10;
    auto random = nova::rng(1);

    // TODO(test-imp): not consistent across platforms, exact match cannot be used
    EXPECT_EQ(
        random.string<nova::alphabetic_distribution>(Length).length(),
        Length
        // random.string<nova::alphabetic_distribution>(Length),
        // "vZLWagpZhm"
    );
}

TEST(Random, LowLessThanEqualHigh_Int_Assertion) {
    EXPECT_THAT(
        []() { nova::random().number(nova::range<int>{ 3, 2 }); },
        testing::ThrowsMessage<nova::exception<void>>(
            testing::HasSubstr("Assertion failed: ")
        )
    );
}

TEST(Random, LowLessThanEqualHigh_FloatNan_Assertion) {
    EXPECT_THROW(nova::random().number(nova::range<float>{ 3.0F, NAN }), nova::exception<void>);
    EXPECT_THROW(nova::random().number(nova::range<float>{ NAN, 3.0F }), nova::exception<void>);
    EXPECT_THROW(nova::random().number(nova::range<float>{ NAN,  NAN }), nova::exception<void>);
}
