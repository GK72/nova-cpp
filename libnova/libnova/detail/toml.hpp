/**
 * Part of Nova C++ Library
 *
 * @brief   TOML documents as `dyn_value` trees.
 *
 * Tables are maps, arrays are sequences. Dates and times have no counterpart.
 * Comments do not survive a round trip.
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

namespace nova::detail::toml {

/**
 * @brief   The TOML document in `text`.
 */
[[nodiscard]] auto parse(std::string_view text) -> expected<dyn_value, error>;

/**
 * @brief   `doc` as a TOML document, ending with a new line.
 */
[[nodiscard]] auto format(const dyn_value& doc) -> expected<std::string, error>;

} // namespace nova::detail::toml
