#include <gmock/gmock.h>

#include "nova/vec.h"

using Vec2 = nova::vec2<float>;
using Vec3 = nova::vec3<float>;
using Vec4 = nova::vec4<float>;
using Vec4i = nova::vec4<int>;

constexpr auto Epsilon = 1e-5;

TEST(Vec, Concept_TypeTraits) {
    using Vec2F = nova::vec<2, float>;
    using Vec7D = nova::vec<7, double>;

    static_assert(nova::is_vec_v<Vec2F>);
    static_assert(nova::is_vec_v<Vec7D>);
    static_assert(nova::vec_like<Vec7D>);
    static_assert(nova::vec_like<Vec3>);
    static_assert(not nova::is_vec_v<Vec3>);
}

TEST(Vec, ElementWiseOperators) {
    constexpr auto u = Vec2(3, 7);
    constexpr auto v = Vec2(2, 4);

    EXPECT_EQ(u + v, Vec2(5, 11));
    EXPECT_EQ(u - v, Vec2(1,  3));
    EXPECT_EQ(u * v, Vec2(6, 28));
    EXPECT_EQ(u / v, Vec2(1.5F, 7.0F / 4.0F));

    constexpr auto p = Vec3(3, 7, 5);
    constexpr auto q = Vec3(2, 4, 3);

    EXPECT_EQ(p + q, Vec3(5, 11, 8));
    EXPECT_EQ(p - q, Vec3(1, 3, 2));
    EXPECT_EQ(p * q, Vec3(6, 28, 15));
    EXPECT_EQ(p / q, Vec3(1.5F, 7.0F / 4.0F, 5.0F / 3.0F));
}

TEST(Vec, FactorOperators) {
    constexpr auto u = Vec2(3, 7);

    EXPECT_EQ(u + 1.0F, Vec2(4, 8));
    EXPECT_EQ(u - 1.0F, Vec2(2, 6));
    EXPECT_EQ(u * 2.0F, Vec2(6, 14));
    EXPECT_EQ(u / 2.0F, Vec2(1.5F, 3.5F));
}

TEST(Vec, MemberOperators) {
    auto u = Vec2(2, 3);

    u += Vec2(5, 2);    EXPECT_EQ(u, Vec2(7, 5));
    u -= Vec2(5, 2);    EXPECT_EQ(u, Vec2(2, 3));
    u *= Vec2(2, 2);    EXPECT_EQ(u, Vec2(4, 6));
    u /= Vec2(2, 2);    EXPECT_EQ(u, Vec2(2, 3));

    u += 1.0F;    EXPECT_EQ(u, Vec2(3, 4));
    u -= 1.0F;    EXPECT_EQ(u, Vec2(2, 3));
    u *= 2.0F;    EXPECT_EQ(u, Vec2(4, 6));
    u /= 2.0F;    EXPECT_EQ(u, Vec2(2, 3));
}

TEST(Vec, Length) {
    EXPECT_NEAR(Vec2(0, 2).length(), 2.0F, Epsilon);
    EXPECT_NEAR(Vec2(3, 4).length(), 5.0F, Epsilon);

    EXPECT_NEAR(Vec3(3, 4, 0).length(), 5.0F, Epsilon);
    EXPECT_NEAR(Vec3(3, 4, 2).length(), 5.38516474F, Epsilon);
}

TEST(Vec, DotProduct) {
    EXPECT_NEAR(nova::dot(Vec2(0, 1), Vec2(1, 0)),          0.0F, Epsilon);
    EXPECT_NEAR(nova::dot(Vec2(1, 2), Vec2(1, 1)),          3.0F, Epsilon);

    EXPECT_NEAR(nova::dot(Vec3(1, 2, 3), Vec3(1, 1, 5)),    18.0F, Epsilon);
}

TEST(Vec, Construction) {
    EXPECT_EQ(
        ( nova::vec<4, float>() ),
        ( nova::vec<4, float>(std::to_array({ 0.0F, 0.0F, 0.0F, 0.0F })) )
    );

    EXPECT_EQ(Vec2(), Vec2(0, 0));
    EXPECT_EQ(Vec3(), Vec3(0, 0, 0));
}

TEST(Vec, Accessors) {
    constexpr auto u = Vec2(2, 3);
    EXPECT_EQ(u.x(), 2);
    EXPECT_EQ(u.y(), 3);

    constexpr auto v = Vec3(2, 3, 4);
    EXPECT_EQ(v.x(), 2);
    EXPECT_EQ(v.y(), 3);
    EXPECT_EQ(v.z(), 4);
}

TEST(Vec, Vec3_Cross) {
    EXPECT_EQ(nova::cross(Vec3(1, 0, 0), Vec3(0, 1, 0)),    Vec3(0, 0,  1));
    EXPECT_EQ(nova::cross(Vec3(0, 1, 0), Vec3(1, 0, 0)),    Vec3(0, 0, -1));
    EXPECT_EQ(nova::cross(Vec3(0, 1, 0), Vec3(0, 2, 0)),    Vec3(0, 0,  0));
}

TEST(Vec, Utilities) {
    EXPECT_EQ(nova::cast8(-1), 0);
    EXPECT_EQ(nova::cast8(128), 128);
    EXPECT_EQ(nova::cast8(256), 255);

    EXPECT_EQ(nova::cast8(-1.0F), 0);
    EXPECT_EQ(nova::cast8(0.5F), 127);
    EXPECT_EQ(nova::cast8(2.0F), 255);

    EXPECT_EQ(nova::pack32BE(Vec4i(    0,    63,   127,   191)), 0x003F7FBF);
    EXPECT_EQ(nova::pack32BE( Vec4(0.00F, 0.25F, 0.50F, 0.75F)), 0x003F7FBF);

    EXPECT_EQ(nova::pack32LE(Vec4i(    0,    63,   127,   191)), 0xBF7F3F00);
    EXPECT_EQ(nova::pack32LE( Vec4(0.00F, 0.25F, 0.50F, 0.75F)), 0xBF7F3F00);

    EXPECT_EQ(nova::product(Vec2(2, 3))   ,  6.0F);
    EXPECT_EQ(nova::product(Vec3(2, 3, 4)), 24.0F);
    EXPECT_EQ(nova::area   (Vec2(2, 3))   ,  6.0F);
    EXPECT_EQ(nova::volume (Vec3(2, 3, 4)), 24.0F);
}
