/**
 * Part of Nova C++ Library
 *
 * @brief   JSON documents as `dyn_value` trees.
 *
 * An integer beyond `std::int64_t` reads as a floating point number.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-10-31
 * @updated     2026-10-31
 * @status      Clean
 */

#pragma once

#include <libnova/dyn_value.hpp>
#include <libnova/error.hpp>
#include <libnova/expected.hpp>

#include <string>
#include <string_view>

namespace nova::detail::json {

/**
 * @brief   The JSON document in `text`.
 */
[[nodiscard]] auto parse(std::string_view text) -> expected<dyn_value, error>;

/**
 * @brief   `doc` as a JSON document, ending with a new line.
 */
[[nodiscard]] auto format(const dyn_value& doc) -> expected<std::string, error>;

} // namespace nova::detail::json
