#include <gtest/gtest.h>

#include "nova/utils.h"

#include <chrono>
#include <type_traits>

TEST(TypeTraits, ChronoDuration) {
    static_assert(nova::is_chrono_duration_v<std::chrono::nanoseconds>);
}

TEST(TypeTraits, ArrayLike) {
    static_assert(nova::is_std_array_v<std::array<int, 1>>);
}

TEST(TypeTraits, VectorLike) {
    static_assert(nova::is_std_vector_v<std::vector<int>>);
}

TEST(TypeTraits, MapLike) {
    static_assert(nova::is_std_map_v<std::map<std::string, int>>);
}
