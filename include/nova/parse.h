/**
 * Part of Nova C++ Library.
 *
 * Parsing utilities. Not inteded to be used in hot loops rather for
 * configuration reading and CLI applications.
 *
 * - Parsing into numbers
 * - Parsing into numbers with metric prefixes (alpha version!)
 * - Parsing into chrono
 */

#pragma once

#include "nova/expected.h"
#include "nova/type_traits.h"

#include <charconv>
#include <stdexcept>
#include <string_view>
#include <type_traits>

#ifdef _LIBCPP_VERSION
#include <string>       // Fallback floating-point parsing
#endif

namespace nova {

enum class parse_error {
    invalid_argument,
    out_of_range
};

#ifndef _LIBCPP_VERSION
/**
 * @brief   Convert a string(-like) to a number.
 */
template <typename R>
[[nodiscard]] auto to_number(std::string_view x) -> expected<R, parse_error> {
    R ret;

    const auto [_, err] = std::from_chars(x.data(), x.data() + x.size(), ret);

    if (err == std::errc::invalid_argument) {
        return unexpected<parse_error>{ parse_error::invalid_argument };
    } else if (err == std::errc::result_out_of_range) {
        return unexpected<parse_error>{ parse_error::out_of_range };
    }

    return ret;
}

#else

/**
 * @brief   Convert a string(-like) to a number.
 *
 * Note: floating-point numbers are not yet supported in libcxx by `std::from_chars()`.
 *
 * ## References
 *
 * - https://libcxx.llvm.org/Status/Cxx17.html#note-p0067
 * - https://en.cppreference.com/w/cpp/compiler_support/17#C.2B.2B17_library_features
 */
template <typename R>
[[nodiscard]] auto to_number(std::string_view x) -> expected<R, parse_error> {
    if constexpr (std::is_integral_v<R>) {
        R ret;

        const auto [_, err] = std::from_chars(x.data(), x.data() + x.size(), ret);

        if (err == std::errc::invalid_argument) {
            return unexpected<parse_error>{ parse_error::invalid_argument };
        } else if (err == std::errc::result_out_of_range) {
            return unexpected<parse_error>{ parse_error::out_of_range };
        }

        return ret;
    } else {
        try {
            if constexpr (std::is_same_v<R, float>) {
                return std::stof(std::string(x));
            } else if constexpr (std::is_same_v<R, double>) {
                return std::stod(std::string(x));
            } else if constexpr (std::is_same_v<R, long double>) {
                return std::stold(std::string(x));
            }
        } catch (const std::invalid_argument& ex) {
            return unexpected<parse_error>{ parse_error::invalid_argument };
        } catch (const std::out_of_range& ex) {
            return unexpected<parse_error>{ parse_error::out_of_range };
        }
    }
}
#endif // _LIBCPP_VERSION

namespace detail {

    /**
     * @brief   Safely convert duration.
     *
     * Ensures that the result is representable by the return (`R`) type.
     */
    template <typename R, typename T>
    [[nodiscard]] auto convert_duration(T x) -> expected<R, parse_error>  {
        using TP = typename T::period;
        using RP = typename R::period;
        constexpr auto ratio_t = static_cast<double>(TP::num) / static_cast<double>(TP::den);
        constexpr auto ratio_r = static_cast<double>(RP::num) / static_cast<double>(RP::den);

        constexpr auto ratio_ratio = static_cast<double>(ratio_t) / static_cast<double>(ratio_r);

        if (static_cast<double>(std::numeric_limits<typename R::rep>::max())
                / ratio_ratio < static_cast<double>(x.count()))
        {
            return unexpected<parse_error>{ parse_error::out_of_range };
        }

        return duration_cast<R>(x);
    }

    /**
     * @brief   Split a string(-like) into a pair of number and a suffix.
     */
    template <typename R>
    [[nodiscard]] auto split_num_n_suffix(std::string_view str)
        -> expected<std::pair<R, std::string_view>, parse_error>
    {
        const auto suffix_iter = std::find_if(
            std::begin(str),
            std::end(str),
            [] (const auto& x) { return std::isalpha(x); }
        );

        const auto suffix_pos = std::distance(std::begin(str), suffix_iter);

        const auto number = to_number<R>(str.substr(0, static_cast<std::size_t>(suffix_pos)));
        if (not number.has_value()) {
            return number.error();
        }

        const auto suffix = str.substr(static_cast<std::size_t>(suffix_pos));

        return std::make_pair(*number, suffix);
    }

} // namespace detail

/**
 * @brief   Convert a string into `chrono`.
 *
 * ## Error
 *
 * Returns an error if the conversion is is out of range for the return type (`R`).
 */
template <typename R> requires chrono_duration<R>
[[nodiscard]] auto to_chrono(const std::string& str) -> expected<R, parse_error> {
    const auto result = detail::split_num_n_suffix<long long>(str);
    if (not result.has_value()) {
        return result.error();
    }

    const auto [number, suffix] = *result;
    using namespace std::chrono;

    if (suffix == "ns") {
        return detail::convert_duration<R>(nanoseconds{ number });
    } else if (suffix == "us") {
        return detail::convert_duration<R>(microseconds{ number });
    } else if (suffix == "ms") {
        return detail::convert_duration<R>(milliseconds{ number });
    } else if (suffix == "s") {
        return detail::convert_duration<R>(seconds{ number });
    } else if (suffix == "min") {
        return detail::convert_duration<R>(minutes{ number });
    } else if (suffix == "h") {
        return detail::convert_duration<R>(hours{ number });
    } else if (suffix == "d") {
        return detail::convert_duration<R>(days{ number });
    } else if (suffix == "w") {
        return detail::convert_duration<R>(weeks{ number });
    } else if (suffix == "M") {
        return detail::convert_duration<R>(months{ number });
    } else if (suffix == "y") {
        return detail::convert_duration<R>(years{ number });
    }

    return unexpected<parse_error>{ parse_error::invalid_argument };
}

namespace alpha {

    /**
     * @brief   Convert a string(-like) number using metric prefixes.
     *
     * NOTE: alpha version!
     *
     * TODO(feat):
     * - safe cast for fractional numbers (disallow integral types)
     * - handle complete list of prefixes
     *
     * ## Example
     *
     * ```
     * auto x = nova::to_number_metric<int>("1k").value();
     * assert(x == 1000);
     * ```
     */
    template <typename R>
    [[nodiscard]] auto to_number_metric(std::string_view str) -> expected<R, parse_error> {
        const auto result = detail::split_num_n_suffix<R>(str);
        if (not result.has_value()) {
            return result.error();
        }

        const auto [number, suffix] = *result;

        if (suffix == "k") {
            return number * static_cast<R>(1000);
        } else if (suffix == "M") {
            return number * static_cast<R>(1'000'000);
        } else if (suffix == "G") {
            return number * static_cast<R>(1'000'000'000);
        } else if (suffix == "T") {
            return number * static_cast<R>(1'000'000'000'000);
        } else if (suffix == "m") {
            return number / static_cast<R>(1000);
        } else if (suffix == "u") {
            return number / static_cast<R>(1'000'000);
        } else if (suffix == "n") {
            return number / static_cast<R>(1'000'000'000);
        }

        return unexpected<parse_error>{ parse_error::invalid_argument };
    }

} // namespace alpha

} // namespace nova
