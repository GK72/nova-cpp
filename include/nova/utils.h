#pragma once

#include "nova/error.h"
#include "nova/intrinsics.h"
#include "nova/types.h"
#include "nova/type_traits.h"

#include <algorithm>
#include <array>
#include <tuple>

namespace nova {

namespace ascii {
    inline constexpr range<char> PrintableRange { 32, 126 };

    // NOLINTBEGIN(*magic-numbers*)
    constexpr auto lowercase_letters() {
        return std::to_array<char>({
             97,  98,  99, 100, 101, 102, 103, 104, 105, 106,
            107, 108, 109, 110, 111, 112, 113, 114, 115, 116,
            117, 118, 119, 120, 121, 122
        });
    }

    constexpr auto uppercase_letters() {
        return std::to_array<char>({
            65, 66, 67, 68, 69, 70, 71, 72, 73, 74,
            75, 76, 77, 78, 79, 80, 81, 82, 83, 84,
            85, 86, 87, 88, 89, 90
        });
    }

    constexpr auto numbers() {
        return std::to_array<char>({
            48, 49, 50, 51, 52, 53, 54, 55, 56, 57
        });
    }
    // NOLINTEND(*magic-numbers*)
} // namespace ascii

#if defined NOVA_WIN
constexpr auto NewLine = "\r\n";
#else
constexpr auto NewLine = '\n';
#endif

/**
 * @brief   Return the current time in UNIX epoch
 */
[[nodiscard]] inline
std::chrono::nanoseconds now() {
    return std::chrono::steady_clock().now().time_since_epoch();
}

/**
 * @brief   Convert a duration to seconds with fractional part
 */
[[nodiscard]] constexpr
auto to_us(chrono_duration auto x) {
    return static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(x).count());
}

template <typename First>
[[nodiscard]] consteval auto concat(First&& first) {
    return first;
}

/**
 * @brief   Compile-time concatenation of arrays
 */
template <typename First, typename Second, typename ...Tail>
[[nodiscard]] consteval auto concat(const First& first, const Second& second, const Tail& ...tail) {
    using T = std::remove_reference_t<First>;
    using value_type = typename T::value_type;

    constexpr auto N = std::tuple_size_v<First> + std::tuple_size_v<Second>;
    std::array<value_type, N> result {};

    std::ranges::copy(first, std::begin(result));
    std::ranges::copy(second, std::next(std::begin(result), std::ssize(first)));
    return concat(result, tail...);
}

} // namespace nova
