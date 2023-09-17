#include <gmock/gmock.h>

#include "nova/vec.h"

constexpr auto Epsilon = 1e-5;

TEST(Vec, Concept_TypeTraits) {
    using Vec7D = nova::vec<7, double>;

    static_assert(nova::is_vec_v<Vec7D>);
    static_assert(nova::vec_like<Vec7D>);

    static_assert(nova::vec_like<nova::Vec2i>);
    static_assert(nova::vec_like<nova::Vec3i>);
    static_assert(nova::vec_like<nova::Vec4i>);

    static_assert(nova::vec_like<nova::Vec2f>);
    static_assert(nova::vec_like<nova::Vec3f>);
    static_assert(nova::vec_like<nova::Vec4f>);

    static_assert(nova::vec_like<nova::Vec2d>);
    static_assert(nova::vec_like<nova::Vec3d>);
    static_assert(nova::vec_like<nova::Vec4d>);

    static_assert(not nova::is_vec_v<nova::Vec3f>);
}

TEST(Vec, ElementWiseOperators) {
    constexpr auto u = nova::Vec2f(3, 7);
    constexpr auto v = nova::Vec2f(2, 4);

    EXPECT_EQ(u + v, nova::Vec2f(5, 11));
    EXPECT_EQ(u - v, nova::Vec2f(1,  3));
    EXPECT_EQ(u * v, nova::Vec2f(6, 28));
    EXPECT_EQ(u / v, nova::Vec2f(1.5F, 7.0F / 4.0F));

    constexpr auto p = nova::Vec3f(3, 7, 5);
    constexpr auto q = nova::Vec3f(2, 4, 3);

    EXPECT_EQ(p + q, nova::Vec3f(5, 11, 8));
    EXPECT_EQ(p - q, nova::Vec3f(1, 3, 2));
    EXPECT_EQ(p * q, nova::Vec3f(6, 28, 15));
    EXPECT_EQ(p / q, nova::Vec3f(1.5F, 7.0F / 4.0F, 5.0F / 3.0F));
}

TEST(Vec, FactorOperators) {
    constexpr auto u = nova::Vec2f(3, 7);

    EXPECT_EQ(u + 1.0F, nova::Vec2f(4, 8));
    EXPECT_EQ(u - 1.0F, nova::Vec2f(2, 6));
    EXPECT_EQ(u * 2.0F, nova::Vec2f(6, 14));
    EXPECT_EQ(u / 2.0F, nova::Vec2f(1.5F, 3.5F));
}

TEST(Vec, MemberOperators) {
    auto u = nova::Vec2f(2, 3);

    u += nova::Vec2f(5, 2);    EXPECT_EQ(u, nova::Vec2f(7, 5));
    u -= nova::Vec2f(5, 2);    EXPECT_EQ(u, nova::Vec2f(2, 3));
    u *= nova::Vec2f(2, 2);    EXPECT_EQ(u, nova::Vec2f(4, 6));
    u /= nova::Vec2f(2, 2);    EXPECT_EQ(u, nova::Vec2f(2, 3));

    u += 1.0F;    EXPECT_EQ(u, nova::Vec2f(3, 4));
    u -= 1.0F;    EXPECT_EQ(u, nova::Vec2f(2, 3));
    u *= 2.0F;    EXPECT_EQ(u, nova::Vec2f(4, 6));
    u /= 2.0F;    EXPECT_EQ(u, nova::Vec2f(2, 3));
}

TEST(Vec, Length) {
    EXPECT_NEAR(nova::Vec2f(0, 2).length(), 2.0F, Epsilon);
    EXPECT_NEAR(nova::Vec2f(3, 4).length(), 5.0F, Epsilon);

    EXPECT_NEAR(nova::Vec3f(3, 4, 0).length(), 5.0F, Epsilon);
    EXPECT_NEAR(nova::Vec3f(3, 4, 2).length(), 5.38516474F, Epsilon);
}

TEST(Vec, DotProduct) {
    EXPECT_NEAR(nova::dot(nova::Vec2f(0, 1), nova::Vec2f(1, 0)),        0.0F, Epsilon);
    EXPECT_NEAR(nova::dot(nova::Vec2f(1, 2), nova::Vec2f(1, 1)),        3.0F, Epsilon);

    EXPECT_NEAR(nova::dot(nova::Vec3f(1, 2, 3), nova::Vec3f(1, 1, 5)),  18.0F, Epsilon);
}

TEST(Vec, Construction) {
    EXPECT_EQ(
        ( nova::vec<4, float>() ),
        ( nova::vec<4, float>(std::to_array({ 0.0F, 0.0F, 0.0F, 0.0F })) )
    );

    EXPECT_EQ(nova::Vec2f(), nova::Vec2f(0, 0));
    EXPECT_EQ(nova::Vec3f(), nova::Vec3f(0, 0, 0));
}

TEST(Vec, Accessors) {
    constexpr auto u = nova::Vec2f(2, 3);
    EXPECT_EQ(u.x(), 2);
    EXPECT_EQ(u.y(), 3);

    constexpr auto v = nova::Vec3f(2, 3, 4);
    EXPECT_EQ(v.x(), 2);
    EXPECT_EQ(v.y(), 3);
    EXPECT_EQ(v.z(), 4);
}

TEST(Vec, Vec3_Cross) {
    EXPECT_EQ(nova::cross(nova::Vec3f(1, 0, 0), nova::Vec3f(0, 1, 0)),    nova::Vec3f(0, 0,  1));
    EXPECT_EQ(nova::cross(nova::Vec3f(0, 1, 0), nova::Vec3f(1, 0, 0)),    nova::Vec3f(0, 0, -1));
    EXPECT_EQ(nova::cross(nova::Vec3f(0, 1, 0), nova::Vec3f(0, 2, 0)),    nova::Vec3f(0, 0,  0));
}

TEST(Vec, UnitVector) {
    constexpr auto scalar2 = std::sqrt(2.0F) / 2.0F;
    constexpr auto scalar3 = std::sqrt(3.0F) / 3.0F;
    EXPECT_EQ(unit(nova::Vec2f(1.0F, 1.0F)),       nova::Vec2f(scalar2, scalar2));
    EXPECT_EQ(unit(nova::Vec3f(1.0F, 1.0F, 1.0F)), nova::Vec3f(scalar3, scalar3, scalar3));
}

TEST(Vec, Utilities) {
    EXPECT_EQ(nova::cast8(-1), 0);
    EXPECT_EQ(nova::cast8(128), 128);
    EXPECT_EQ(nova::cast8(256), 255);

    EXPECT_EQ(nova::cast8(-1.0F), 0);
    EXPECT_EQ(nova::cast8(0.5F), 127);
    EXPECT_EQ(nova::cast8(2.0F), 255);

    EXPECT_EQ(nova::pack32BE(nova::Vec4i(    0,    63,   127,   191)), 0x003F7FBF);
    EXPECT_EQ(nova::pack32BE(nova::Vec4f(0.00F, 0.25F, 0.50F, 0.75F)), 0x003F7FBF);

    EXPECT_EQ(nova::pack32LE(nova::Vec4i(    0,    63,   127,   191)), 0xBF7F3F00);
    EXPECT_EQ(nova::pack32LE(nova::Vec4f(0.00F, 0.25F, 0.50F, 0.75F)), 0xBF7F3F00);

    EXPECT_EQ(nova::product(nova::Vec2f(2, 3))   ,  6.0F);
    EXPECT_EQ(nova::product(nova::Vec3f(2, 3, 4)), 24.0F);
    EXPECT_EQ(nova::area   (nova::Vec2f(2, 3))   ,  6.0F);
    EXPECT_EQ(nova::volume (nova::Vec3f(2, 3, 4)), 24.0F);
}
