#include <gtest/gtest.h>

#include "nova/utils.h"

#include <chrono>
#include <type_traits>

TEST(TypeTraits, ChronoDuration) {
    static_assert(nova::is_chrono_duration_v<std::chrono::nanoseconds>);
}
