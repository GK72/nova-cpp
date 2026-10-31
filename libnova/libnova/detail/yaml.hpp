/**
 * Part of Nova C++ Library
 *
 * @brief   YAML documents as `dyn_value` trees.
 *
 * Plain scalars resolve by the YAML 1.2 core schema: `true`/`false` are bools,
 * integers (decimal, `0x` hex, `0o` octal) are integers, floats (`.inf`, `.nan`
 * too) are floating point numbers, `null`/`~`/empty are nulls, everything else
 * is a string. Quoted scalars are always strings.
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

namespace nova::detail::yaml {

/**
 * @brief   The YAML document in `text`.
 */
[[nodiscard]] auto parse(std::string_view text) -> expected<dyn_value, error>;

/**
 * @brief   `doc` as a YAML document, ending with a new line.
 */
[[nodiscard]] auto format(const dyn_value& doc) -> expected<std::string, error>;

} // namespace nova::detail::yaml
