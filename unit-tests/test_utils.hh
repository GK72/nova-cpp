/**
 * Nova test utilities.
 */

#include "nova/error.hh"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <compare>

#define EXPECT_ASSERTION_FAIL(expr)                                            \
    EXPECT_THAT(                                                               \
        [](){ expr; },                                                         \
        testing::ThrowsMessage<nova::exception>(testing::StartsWith("Assertion failed: ")))

/**
 * @brief   Non-trivial type.
 */
struct nt {
    constexpr nt(int p = 666) : x(p) {}

    constexpr nt(const nt&)            = default;
    constexpr nt& operator=(const nt&) = default;
    constexpr nt(nt&&)                 = default;
    constexpr nt& operator=(nt&&)      = default;

    constexpr ~nt() { /* Non-trivially destructible */ }

    constexpr auto operator<=>(const nt&) const = default;

    int x;
};

/**
 * @brief   Non-copyable, moveable only type.
 */
struct moo {
    constexpr moo(int p = 666) : x(p) {}

    constexpr moo(const moo&)            = delete;
    constexpr moo& operator=(const moo&) = delete;
    constexpr moo(moo&&)                 = default;
    constexpr moo& operator=(moo&&)      = default;

    constexpr ~moo() = default;

    constexpr auto operator<=>(const moo&) const = default;

    int x;
};

/**
 * @brief   Non-copyable, non-moveable type.
 */
struct nomoo {
    constexpr nomoo(int p = 666) : x(p) {}

    constexpr nomoo(const nomoo&)            = delete;
    constexpr nomoo& operator=(const nomoo&) = delete;
    constexpr nomoo(nomoo&&)                 = delete;
    constexpr nomoo& operator=(nomoo&&)      = delete;

    constexpr ~nomoo() = default;

    constexpr auto operator<=>(const nomoo&) const = default;

    int x;
};
