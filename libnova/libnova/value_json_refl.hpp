/**
 * Part of Nova C++ Library
 *
 * Convenience glue chaining the reflection-based binder (`value_refl.hpp`)
 * with JSON (de)serialization (`value_json.hpp`), so a typed aggregate can
 * round-trip through JSON text directly.
 *
 * Requires `NOVA_EXPERIMENTAL_FEATURE_SET` (reflection); this entire header
 * is a no-op without it.
 *
 * @author  Gábor Krisztián Girhiny
 * @date    2026-10-03
 */

#pragma once

#ifdef NOVA_EXPERIMENTAL_FEATURE_SET

#include <libnova/value_json.hpp>
#include <libnova/value_refl.hpp>

#include <string>
#include <string_view>

namespace nova {

/**
 * @brief   Parses `text` as JSON and binds it onto a `T`.
 */
template <typename T, typename Traits = default_value_traits>
auto bind_json(std::string_view text) -> T {
    return bind<T>(parse_json<Traits>(text));
}

/**
 * @brief   The reverse of `bind_json`: turn a `T` into JSON text.
 */
template <typename Traits = default_value_traits, typename T>
auto unbind_json(const T& t, int indent = -1) -> std::string {
    return dump_json(unbind<Traits>(t), indent);
}

} // namespace nova

#endif // NOVA_EXPERIMENTAL_FEATURE_SET
