#include "nova/utils.hh"

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <map>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

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

TEST(TypeTraits, StringLike) {
    static_assert(not nova::string_like<int>);
    static_assert(nova::string_like<char*>);
    static_assert(nova::string_like<std::string>);
    static_assert(nova::string_like<std::string_view>);
}
