#include <gtest/gtest.h>

#include "nova/allocator.h"
#include "nova/data.h"
#include "nova/intrinsics.h"

#include <cstddef>
#include <span>
#include <vector>

TEST(Allocator, ph) {
    auto buffer = std::vector<std::byte>(32);
    auto mem = nova::mem(buffer);

    auto xs = std::pmr::vector<int>({ 1, 2, 3, 4 }, &mem);

#ifdef NOVA_WIN
    EXPECT_EQ(mem.total_allocations(), 2);
    EXPECT_EQ(mem.total_allocated_bytes(), 32);
    EXPECT_EQ(
        nova::data_view(std::span(buffer)).as_hex_string(16),
        "01000000020000000300000004000000"
    );
#else
    EXPECT_EQ(mem.total_allocations(), 1);
    EXPECT_EQ(mem.total_allocated_bytes(), 16);
#endif

    EXPECT_EQ(xs.at(2), 3);

    const auto ptr = nova::data_view(std::span(buffer)).as_hex_string(0, 16);

    mem.release();
    EXPECT_EQ(
        nova::data_view(std::span(buffer)).as_hex_string(16),
        "01000000020000000300000004000000"
    );

    auto xs2 = std::pmr::vector<int>({ 11, 12, 13, 14 }, &mem);
    EXPECT_EQ(xs.at(2), 3);

    const auto ptr2 = nova::data_view(std::span(buffer)).as_hex_string(0, 16);
    EXPECT_EQ(ptr, ptr2);
}
