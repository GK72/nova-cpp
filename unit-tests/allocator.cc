#include <gtest/gtest.h>

#include "nova/allocator.h"

#include <cstddef>
#include <vector>

TEST(Allocator, ph) {
    auto buffer = std::vector<std::byte>(32);
    auto mem = nova::mem(buffer);

    auto xs = std::pmr::vector<int>(4, &mem);

    EXPECT_EQ(mem.total_allocations(), 1);
    EXPECT_EQ(mem.total_allocated_bytes(), 16);
}
