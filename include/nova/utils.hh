/**
 * Part of Nova C++ Library.
 *
 * Various utility functions, types and constants.
 */

#pragma once

#include "nova/error.hh"
#include "nova/expected.hh"
#include "nova/types.hh"
#include "nova/type_traits.hh"

#include <fmt/format.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <concepts>
#include <cstdlib>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include <stdlib.h>

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
 * @brief   Split a string by delimiter.
 *
 * @param str       Input string to split.
 * @param delim     Delimiter string which can consist of multiple characters.
 *
 * @returns         A vector of strings.
 */
[[nodiscard]] inline auto split(std::string_view str, std::string_view delim) {
    auto temp = std::views::split(str, delim)
              | std::views::transform([](const auto& elem) {
                  return std::string(std::ranges::begin(elem), std::ranges::end(elem));
              });
    return std::vector<std::string>(std::begin(temp), std::end(temp));
}

/**
 * @brief   Return the current time in UNIX epoch.
 */
[[nodiscard]] inline
std::chrono::nanoseconds now() {
    return std::chrono::steady_clock().now().time_since_epoch();
}

/**
 * @brief   Convert a duration to seconds with fractional part.
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
 * @brief   Compile-time concatenation of arrays.
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

[[nodiscard]] inline auto getenv(const std::string& env_name) -> exp::expected<std::string, error> {
#ifdef NOVA_WIN
    char* env;
    // size_t len;
    errno_t err = _dupenv_s(&env, nullptr, env_name.c_str());

    if (env == nullptr) {
        return { exp::unexpect,fmt::format("Environment variable is not set: {}", env_name) };
    }

    if (err) {
        return { exp::unexpect,fmt::format("Error querying environment variable: {}", env_name) };
    }

    const auto ret = std::string{ env };
    free(env);
    return ret;

#else
    char* env = std::getenv(env_name.c_str());
    if (env == nullptr) {
        return { exp::unexpect, fmt::format("Environment variable is not set: {}", env_name) };
    }
    return { env };
#endif
}

[[nodiscard]] inline auto getenv(const std::string& env_name, const std::string& def) -> std::string {
    const auto env = getenv(env_name);
    if (not env.has_value()) {
        return def;
    }
    return *env;
}

/**
 * @brief   Generate evenly spaced numbers over the range.
 */
template <std::floating_point R = float, typename T>
[[nodiscard]] auto linspace(range<T> range, std::size_t num, bool inclusive = true) -> std::vector<R> {
    auto ret = std::vector<R>(num);

    const auto interval = (static_cast<R>(range.high) - static_cast<R>(range.low)) /
        (static_cast<R>(num) - (1 * static_cast<R>(inclusive)));

    R value = static_cast<R>(range.low);

    for (R& x : ret) {
        x = value;
        value += interval;
    }

    return ret;
}

/**
 * @brief   A simple stopwatch measuring in nanosecond resolution.
 */
class stopwatch {
public:
    [[nodiscard]] stopwatch()
        : m_clock(now())
    {}

    /**
     * @brief   Measure the elapsed time since construction.
     */
    [[nodiscard]] auto elapsed() -> std::chrono::nanoseconds {
        return now() - m_clock;
    }

    /**
     * @brief   Measure the elapsed time since last call this function.
     */
    auto lap() -> std::chrono::nanoseconds {
        const auto time = now();
        const auto ret = time - m_clock;
        m_clock = time;
        return ret;
    }

private:
    std::chrono::nanoseconds m_clock;
};

} // namespace nova
