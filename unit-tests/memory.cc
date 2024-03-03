#include <gtest/gtest.h>

#include "nova/data.h"
#include "nova/memory.h"
#include "nova/intrinsics.h"

#include <cstddef>
#include <span>
#include <vector>

TEST(Memory, MemoryResource_Allocations) {
    auto buffer = std::vector<std::byte>(32);
    auto mem = nova::mem(buffer);
    const auto xs = std::pmr::vector<int>({ 99 }, &mem);

    auto expected_allocations = 1;
    auto expected_allocated_bytes = 4;

#if defined(NOVA_WIN) && not defined(NDEBUG)
    // Container proxy object: https://github.com/microsoft/STL/issues/2170
    expected_allocations += 1;
    expected_allocated_bytes += 16;
#endif

    EXPECT_EQ(mem.total_allocations(), expected_allocations);
    EXPECT_EQ(mem.total_allocated_bytes(), expected_allocated_bytes);
    EXPECT_EQ(mem.remaining_capacity(), expected_allocated_bytes);
}

TEST(Memory, ph) {
    auto factory = nova::mem_factory(32);
    auto xs = factory.vector<int>();
    auto& buffer = factory.last_buffer();
}
