/**
 * Part of Nova C++ Library
 *
 * JSON (de)serialization for `nova::basic_value` attribute trees.
 *
 * Reflection-free, like `value.hpp` itself: this only walks the variant,
 * it doesn't need to know about any user aggregate type.
 *
 * @author  Gábor Krisztián Girhiny
 * @date    2026-10-03
 */

#pragma once

#include <libnova/error.hpp>
#include <libnova/value.hpp>

#define JSON_DIAGNOSTICS 1

#ifdef NOVA_GCC
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=106247
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#include <nlohmann/json.hpp>
#pragma GCC diagnostic pop
#else
#include <nlohmann/json.hpp>
#endif

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace nova {

/**
 * @brief   Converts a `basic_value` tree into an `nlohmann::json` document.
 */
template <typename Traits>
auto to_json(const basic_value<Traits>& v) -> nlohmann::json {
    return std::visit(
        []<typename U>(const U& contained) -> nlohmann::json {
            if constexpr (std::same_as<U, std::monostate>) {
                return nullptr;
            } else if constexpr (std::same_as<U, typename basic_value<Traits>::seq_t>) {
                auto out = nlohmann::json::array();
                for (const auto& elem : contained) {
                    out.push_back(to_json(elem));
                }
                return out;
            } else if constexpr (std::same_as<U, typename basic_value<Traits>::map_t>) {
                auto out = nlohmann::json::object();
                for (const auto& [key, val] : contained) {
                    out[key] = to_json(val);
                }
                return out;
            } else {
                return contained;
            }
        }
        , v.data()
    );
}

/**
 * @brief   Converts an `nlohmann::json` document into a `basic_value` tree.
 */
template <typename Traits = default_value_traits>
auto from_json(const nlohmann::json& j) -> basic_value<Traits> {
    using V = basic_value<Traits>;
    using string_t = typename V::union_t::string_t;

    if (j.is_null()) {
        return V();
    }

    if (j.is_boolean()) {
        return V(j.template get<bool>());
    }

    if (j.is_number_integer()) {
        return V(j.template get<std::int64_t>());
    }

    if (j.is_number_float()) {
        return V(j.template get<double>());
    }

    if (j.is_string()) {
        return V(string_t(j.template get<std::string>()));
    }

    if (j.is_array()) {
        typename V::seq_t seq;
        for (const auto& elem : j) {
            seq.push_back(from_json<Traits>(elem));
        }
        return V(std::move(seq));
    }

    if (j.is_object()) {
        typename V::map_t map;
        for (const auto& [key, val] : j.items()) {
            map.emplace(key, from_json<Traits>(val));
        }
        return V(std::move(map));
    }

    throw exception("Cannot convert JSON value to attribute value");
}

/**
 * @brief   Parses `text` into a `basic_value` tree.
 */
template <typename Traits = default_value_traits>
auto parse_json(std::string_view text) -> basic_value<Traits> {
    try {
        return from_json<Traits>(nlohmann::json::parse(text));
    } catch (const nlohmann::json::exception& ex) {
        throw exception("Parsing error: {}", ex.what());
    }
}

/**
 * @brief   Dumps a `basic_value` tree as JSON text.
 *
 * @param   indent  Pretty-prints with the given indent width; the default (-1)
 *                   produces compact, single-line output (`nlohmann::json`'s convention).
 */
template <typename Traits>
auto dump_json(const basic_value<Traits>& v, int indent = -1) -> std::string {
    return to_json(v).dump(indent);
}

} // namespace nova
