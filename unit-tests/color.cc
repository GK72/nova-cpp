#include <gtest/gtest.h>

#include "nova/color.h"

#include <type_traits>

constexpr auto Epsilon = 1e-5;

TEST(Color, ColorScale) {
    static_assert(std::is_same_v<nova::Color, nova::color<nova::color_scale::Normalized>>);

    constexpr auto color1 = nova::color<nova::color_scale::Normalized>{ 255, 127, 0, 0 };
    EXPECT_NEAR(color1.r(), 1.0F     , Epsilon);
    EXPECT_NEAR(color1.g(), 0.498039F, Epsilon);
    EXPECT_NEAR(color1.b(), 0.0F     , Epsilon);
    EXPECT_NEAR(color1.a(), 0.0F     , Epsilon);

    constexpr auto color2 = nova::color<nova::color_scale::Scaled>{ 1.0F, 0.5F, 0.0F, 0.0F };
    EXPECT_NEAR(color2.r(), 255.0F, Epsilon);
    EXPECT_NEAR(color2.g(), 127.5F, Epsilon);
    EXPECT_NEAR(color2.b(),   0.0F, Epsilon);
    EXPECT_NEAR(color2.a(),   0.0F, Epsilon);

    EXPECT_EQ(nova::colors::black.a(), 1);
}
