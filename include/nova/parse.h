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

namespace nova {

enum class parse_error {
    invalid_argument,
    lossy_conversion,
    out_of_range
};

/**
 * @brief   Convert a string(-like) to a number.
 */
template <typename R>
[[nodiscard]] auto to_number(std::string_view x) -> expected<R, parse_error> {
    R ret;

    const auto [_, err] = std::from_chars(x.data(), x.data() + x.size(), ret);

    if (err == std::errc::invalid_argument) {
        return unexpected{ parse_error::invalid_argument };
    } else if (err == std::errc::result_out_of_range) {
        return unexpected{ parse_error::out_of_range };
    }

    return ret;
}

namespace detail {

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

} // namespace nova
