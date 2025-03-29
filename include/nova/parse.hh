/**
 * Part of Nova C++ Library.
 *
 * Parsing utilities. Not intended to be used in hot loops rather for
 * configuration reading and CLI applications.
 *
 * - Parsing into numbers with metric prefixes
 * - Parsing into chrono
 */

#pragma once

#include "nova/expected.hh"
#include "nova/intrinsics.hh"
#include "nova/type_traits.hh"

#include <charconv>
#include <cstdint>
#include <limits>
#include <ratio>
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

namespace detail {

    template <typename T>
        requires std::is_integral_v<T>
    [[nodiscard]] auto safe_multiply(T lhs, std::intmax_t rhs) -> expected<T, parse_error> {
        constexpr auto limit = std::numeric_limits<T>::max();

        if (limit / lhs < rhs) {
            return unexpected{ parse_error::out_of_range };
        }
        return lhs * static_cast<T>(rhs);
    }

    template <typename T>
        requires std::is_floating_point_v<T>
    [[nodiscard]] auto safe_multiply(T lhs, std::intmax_t rhs) -> expected<T, parse_error> {
        constexpr auto limit = std::numeric_limits<T>::max();

        if (limit / lhs < static_cast<T>(rhs)) {
            return unexpected{ parse_error::out_of_range };
        }
        return lhs * static_cast<T>(rhs);
    }

    template <typename Num, typename Den>
        requires std::is_floating_point_v<Num> and std::is_arithmetic_v<Den>
    [[nodiscard]] auto safe_division(Num lhs, Den rhs) -> expected<Num, parse_error> {
        return lhs / static_cast<Num>(rhs);
    }

    template <typename Num, typename Den>
    [[nodiscard]] auto safe_division(Num lhs, Den rhs) -> expected<Num, parse_error> NOVA_DELETE("Only floating point numbers are supported as safe division");

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
        }
        if (err == std::errc::result_out_of_range) {
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
     * @references
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

    /**
     * @brief   Safely convert duration.
     *
     * Ensures that the result is representable by the return type `R`.
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
     * @brief   Check if the character can be a number suffix.
     *
     * For example, "1M" = one million.
     *
     * Floating point exponents contain alphabetic characters, but
     * they are part of the number, e.g., "1.1e+10".
     */
    [[nodiscard]] inline
    auto is_number_suffix(char x) -> bool {
        return std::isalpha(x) != 0
            && x != 'e'
            && x != '+'
            && x != '-';
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
            [] (const auto& x) { return is_number_suffix(x); }
        );

        const auto suffix_pos = std::distance(std::begin(str), suffix_iter);

        const auto number = detail::to_number<R>(str.substr(0, static_cast<std::size_t>(suffix_pos)));
        if (not number.has_value()) {
            return unexpected{ number.error() };
        }

        const auto suffix = str.substr(static_cast<std::size_t>(suffix_pos));

        return std::make_pair(*number, suffix);
    }

} // namespace detail

/**
 * @brief   Convert a string into `chrono`.
 *
 * @error   if the conversion is out of range for the return type `R`.
 */
template <typename R> requires chrono_duration<R>
[[nodiscard]] auto to_chrono(const std::string& str) -> expected<R, parse_error> {
    const auto result = detail::split_num_n_suffix<typename R::rep>(str);
    if (not result.has_value()) {
        return unexpected{ result.error() };
    }

    const auto [number, suffix] = *result;
    using namespace std::chrono;

    if (suffix == "ns")  { return detail::convert_duration<R>(nanoseconds{ number }); }
    if (suffix == "us")  { return detail::convert_duration<R>(microseconds{ number }); }
    if (suffix == "ms")  { return detail::convert_duration<R>(milliseconds{ number }); }
    if (suffix == "s")   { return detail::convert_duration<R>(seconds{ number }); }
    if (suffix == "min") { return detail::convert_duration<R>(minutes{ number }); }
    if (suffix == "h")   { return detail::convert_duration<R>(hours{ number }); }
    if (suffix == "d")   { return detail::convert_duration<R>(days{ number }); }
    if (suffix == "w")   { return detail::convert_duration<R>(weeks{ number }); }
    if (suffix == "M")   { return detail::convert_duration<R>(months{ number }); }
    if (suffix == "y")   { return detail::convert_duration<R>(years{ number }); }

    return unexpected<parse_error>{ parse_error::invalid_argument };
}

/**
 * @brief   Convert a string(-like) number.
 *
 * Supports metric prefixes from exa to atto.
 *
 * Floating point exponents are represented with the letter small "e".
 * Big "E" is for exa.
 *
 * Note: micro is letter "u". Micro sign (U+00B5) is not supported.
 *
 * @error   if the result cannot be exactly represented in the result type `R`.
 *
 * IMPORTANT: Bases with negative exponents, i.e. everything smaller than
 * deca always returns an error for integral types.
 *
 * @example
 *
 * ```
 * auto x = nova::to_number<int>("1k").value();
 * assert(x == 1000);
 * ```
 */
template <typename R>
    requires std::is_integral_v<R> or std::is_floating_point_v<R>
[[nodiscard]] auto to_number(std::string_view str) -> expected<R, parse_error> {                    // NOLINT(readability-function-cognitive-complexity) | `if` statements as switch-case.
    const auto result = detail::split_num_n_suffix<R>(str);
    if (not result.has_value()) {
        return unexpected{ result.error() };
    }

    const auto [number, suffix] = *result;

    if (suffix == "") {
        return number;
    }

    if (suffix == "E") { return detail::safe_multiply(number, std::exa::num); }
    if (suffix == "P") { return detail::safe_multiply(number, std::peta::num); }
    if (suffix == "T") { return detail::safe_multiply(number, std::tera::num); }
    if (suffix == "G") { return detail::safe_multiply(number, std::giga::num); }
    if (suffix == "M") { return detail::safe_multiply(number, std::mega::num); }
    if (suffix == "k") { return detail::safe_multiply(number, std::kilo::num); }
    if (suffix == "h") { return detail::safe_multiply(number, std::hecto::num); }
    if (suffix =="da") { return detail::safe_multiply(number, std::deca::num); }

    if constexpr (std::is_integral_v<R>) {
        return unexpected<parse_error>{ parse_error::invalid_argument };
    } else {
        if (suffix == "d") { return detail::safe_division(number, std::deci::den); }
        if (suffix == "c") { return detail::safe_division(number, std::centi::den); }
        if (suffix == "m") { return detail::safe_division(number, std::milli::den); }
        if (suffix == "u") { return detail::safe_division(number, std::micro::den); }
        if (suffix == "n") { return detail::safe_division(number, std::nano::den); }
        if (suffix == "p") { return detail::safe_division(number, std::pico::den); }
        if (suffix == "f") { return detail::safe_division(number, std::femto::den); }
        if (suffix == "a") { return detail::safe_division(number, std::atto::den); }

        return unexpected<parse_error>{ parse_error::invalid_argument };
    }
}

} // namespace nova
