#define NOVA_RUNTIME_ASSERTIONS

#include "test_utils.hh"

#include "nova/not_null.hh"

#include <gtest/gtest.h>

#include <cstdint>
#include <type_traits>

#include <memory>

TEST(NotNull, Meta_ORT_TrivialType) {
    static_assert(std::is_same_v<nova::detail::ort<int>, const int>);
}

TEST(NotNull, Meta_ORT_SmallCompoundType) {
    struct S {
        std::uint32_t x;
        std::uint32_t y;
        std::uint32_t z;
    };

    static_assert(sizeof(S) == 12);
    static_assert(std::is_same_v<nova::detail::ort<S>, const S>);
}

TEST(NotNull, Meta_ORT_NonTriviallyCopyable) {
    // Non-trivial destructor
    static_assert(std::is_same_v<nova::detail::ort<nt>, const nt&>);
}

TEST(NotNull, RawPtr_Nullptr) {
    int* ptr = nullptr;
    EXPECT_ASSERTION_FAIL(nova::not_null{ ptr });
}

TEST(NotNull, RawPtr) {
    int x = 9;
    auto ptr = nova::not_null{ &x };
    EXPECT_EQ(*ptr, 9);
}

TEST(NotNull, UniquePtr_And_Move) {
    auto ptr = nova::not_null{ std::make_unique<int>(9) };
    EXPECT_EQ(*ptr, 9);

    auto ptr2 = std::move(ptr);
    EXPECT_EQ(*ptr2, 9);
}

TEST(NotNull, Copy) {
    int x = 9;
    auto ptr = nova::not_null{ &x };
    auto ptr2 = nova::not_null{ ptr };
    auto ptr3 = ptr;
    EXPECT_EQ(*ptr2, 9);
    EXPECT_EQ(*ptr3, 9);
}

TEST(NotNull, RawPtr_ImplicitConversion) {
    int x = 9;
    auto ptr = nova::not_null{ &x };
    int* y = ptr;
    EXPECT_EQ(*y, 9);
}

TEST(NotNull, RawPtr_Compound) {
    auto x = nt{ 9 };
    auto ptr = nova::not_null{ &x };
    EXPECT_EQ(ptr->x, 9);
}

TEST(NotNull, RawPtr_MoveableOnly) {
    auto x = moo{ 9 };
    auto ptr = nova::not_null{ &x };
    EXPECT_EQ(ptr->x, 9);
}

TEST(NotNull, RawPtr_NonMoveable) {
    auto x = nomoo{ 9 };
    auto ptr = nova::not_null{ &x };
    EXPECT_EQ(ptr->x, 9);
}
