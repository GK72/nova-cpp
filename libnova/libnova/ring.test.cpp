/**
 * Part of Nova C++ Library.
 *
 * @brief   Tests of `ring.hpp`.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2024-10-27
 * @updated     2026-11-07
 * @reviewed    read
 * SPDX-License-Identifier: BSL-1.0
 */

#define NOVA_RUNTIME_ASSERTIONS

#include <libnova/ring.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <initializer_list>
#include <iterator>
#include <tuple>
#include <vector>

using ::testing::ElementsAre;

namespace {

    [[nodiscard]] auto ring_of(std::size_t size, std::initializer_list<int> values) -> nova::ring<int> {
        auto ret = nova::ring<int>(size);
        for (const auto v : values) {
            ret.push_back(v);
        }
        return ret;
    }

    [[nodiscard]] auto storage(const nova::ring<int>& r) -> std::vector<int> {
        return { r.data(), r.data() + r.size() };
    }

    [[nodiscard]] auto elements(const nova::ring<int>& r) -> std::vector<int> {
        return { r.begin(), r.end() };
    }

} // namespace

TEST(Ring, PushBack_WrapsAround) {
    const auto xs = ring_of(3, { 1, 2, 3, 4 });

    EXPECT_THAT(storage(xs), ElementsAre(4, 2, 3));
    EXPECT_EQ(xs.offset(), 1u);
    EXPECT_EQ(xs.back(), 4);
}

TEST(Ring, Resize_ShrinkKeepsTheNewest) {
    auto xs = ring_of(4, { 1, 2, 3, 4, 5, 6 });
    xs.resize(2);

    EXPECT_THAT(storage(xs), ElementsAre(5, 6));
    EXPECT_EQ(xs.back(), 6);

    xs.push_back(7);
    EXPECT_THAT(storage(xs), ElementsAre(7, 6));
}

TEST(Ring, Resize_ShrinkBelowTheWriteIndex) {
    auto xs = ring_of(5, { 1, 2, 3, 4 });
    xs.resize(2);

    EXPECT_THAT(storage(xs), ElementsAre(3, 4));
    EXPECT_LT(xs.offset(), xs.size());

    xs.push_back(5);
    EXPECT_THAT(storage(xs), ElementsAre(5, 4));
}

TEST(Ring, Resize_GrowKeepsTheOrder) {
    auto xs = ring_of(3, { 1, 2, 3, 4 });
    xs.resize(5);

    EXPECT_THAT(storage(xs), ElementsAre(2, 3, 4, 0, 0));

    xs.push_back(5);
    EXPECT_THAT(storage(xs), ElementsAre(2, 3, 4, 5, 0));
}

TEST(Ring, Resize_NotFullKeepsEmptySlotsAtTheEnd) {
    auto xs = ring_of(4, { 1, 2 });
    xs.resize(3);

    EXPECT_THAT(storage(xs), ElementsAre(1, 2, 0));
    EXPECT_EQ(xs.back(), 2);
}

TEST(Ring, Clear_KeepsTheSize) {
    auto xs = ring_of(3, { 1, 2 });
    xs.clear();

    EXPECT_THAT(storage(xs), ElementsAre(0, 0, 0));

    xs.push_back(9);
    EXPECT_THAT(storage(xs), ElementsAre(9, 0, 0));
}

TEST(Ring, SizeZero_KeepsNothing) {
    auto xs = ring_of(0, { 1, 2 });
    EXPECT_EQ(xs.size(), 0u);

    xs.resize(2);
    xs.push_back(3);
    EXPECT_THAT(storage(xs), ElementsAre(3, 0));
}

TEST(Ring, SizeZero_BackAsserts) {
    const auto xs = nova::ring<int>(0);
    EXPECT_THROW(std::ignore = xs.back(), nova::exception);
}

TEST(Ring, Iteration_FromTheOldestToTheNewest) {
    static_assert(std::forward_iterator<nova::ring<int>::iterator>);
    static_assert(std::forward_iterator<nova::ring<int>::const_iterator>);

    EXPECT_THAT(elements(ring_of(3, {})), ElementsAre());
    EXPECT_THAT(elements(ring_of(3, { 1, 2 })), ElementsAre(1, 2));
    EXPECT_THAT(elements(ring_of(3, { 1, 2, 3 })), ElementsAre(1, 2, 3));
    EXPECT_THAT(elements(ring_of(3, { 1, 2, 3, 4, 5 })), ElementsAre(3, 4, 5));
    EXPECT_THAT(elements(ring_of(0, { 1 })), ElementsAre());
}

TEST(Ring, Iteration_AfterResizeAndClear) {
    auto r = ring_of(4, { 1, 2, 3, 4, 5, 6 });
    r.resize(3);
    EXPECT_THAT(elements(r), ElementsAre(4, 5, 6));

    r.resize(5);
    r.push_back(7);
    EXPECT_THAT(elements(r), ElementsAre(4, 5, 6, 7));

    r.clear();
    EXPECT_THAT(elements(r), ElementsAre());
}

TEST(Ring, Iteration_Mutable) {
    auto r = ring_of(2, { 1, 2, 3 });
    for (auto& v : r) {
        v *= 10;
    }
    EXPECT_THAT(elements(r), ElementsAre(20, 30));
}
