/**
 * Part of Nova C++ Library
 *
 * @brief   JSON reader and writer.
 *
 * nlohmann_json does nearly everything: parsing, with syntax errors and their
 * positions, typed values (JSON is typed, unlike YAML), and writing with
 * indentation, escaping, and floats in their shortest exact form (`2.0` stays
 * `2.0`). Of repeated keys, the last wins.
 *
 * Special handling is implemented for:
 * - Reading an unsigned integer beyond `std::int64_t` as a float, as YAML
 *   does: a `dyn_value` has signed integers only.
 * - Rejecting infinities and NaNs when writing: JSON has no counterpart, and
 *   nlohmann_json writes them as `null` without a word, which would silently
 *   change the document.
 * - The layout: four spaces of indentation and a final new line.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-10-31
 * @updated     2026-10-31
 * @status      Prototype
 */

#include <libnova/detail/json.hpp>
#include <libnova/detail/collect.hpp>

#include <libnova/dyn_value.hpp>
#include <libnova/error.hpp>
#include <libnova/expected.hpp>

#include <fmt/format.h>

#ifdef NOVA_GCC
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=106247
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#include <nlohmann/json.hpp>
#pragma GCC diagnostic pop
#else
#include <nlohmann/json.hpp>
#endif

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <string>
#include <map>
#include <string_view>
#include <utility>
#include <vector>
#include <variant>

namespace nova::detail::json {

namespace {

    using json_t = nlohmann::json;

    [[nodiscard]] auto from_json(const json_t& node) -> dyn_value;

    /**
     * @brief   Beyond `std::int64_t`, a floating point number.
     */
    [[nodiscard]] auto from_unsigned(std::uint64_t number) -> dyn_value {
        constexpr auto max = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
        return number > max
            ? dyn_value(static_cast<double>(number))
            : dyn_value(static_cast<std::int64_t>(number));
    }

    [[nodiscard]] auto from_array(const json_t& node) -> dyn_value {
        auto ret = dyn_value::seq_t{ };
        ret.reserve(node.size());

        std::ranges::transform(
            node,
            std::back_inserter(ret),
            [](const json_t& item) { return from_json(item); }
        );

        return ret;
    }

    [[nodiscard]] auto from_object(const json_t& node) -> dyn_value {
        auto ret = dyn_value::map_t{ };

        std::ranges::for_each(
            node.items(),
            [&ret](const auto& entry) {
                ret.insert_or_assign(
                    entry.key(),
                    from_json(entry.value())
                );
            }
        );

        return ret;
    }

    /**
     * @brief   Every JSON value has a counterpart: reading cannot fail.
     */
    [[nodiscard]] auto from_json(const json_t& node) -> dyn_value {
        using enum json_t::value_t;

        switch (node.type()) {
            case number_integer:    return node.get<std::int64_t>();
            case number_unsigned:   return from_unsigned(node.get<std::uint64_t>());
            case number_float:      return node.get<double>();
            case boolean:           return node.get<bool>();
            case string:            return node.get_ref<const std::string&>();
            case array:             return from_array(node);
            case object:            return from_object(node);
            case null:
            case binary:            // Never parsed from text
            case discarded:         // Only by a parser callback
                break;
        }

        return { };
    }

    [[nodiscard]] auto to_json(const dyn_value& node, const dyn_path& path) -> expected<json_t, error>;

    [[nodiscard]] auto to_json(std::monostate /* null */, const dyn_path& /* path */) -> expected<json_t, error> {
        return json_t(nullptr);
    }

    [[nodiscard]] auto to_json(double number, const dyn_path& path) -> expected<json_t, error> {
        if (not std::isfinite(number)) {
            return wrap_error(dyn_value::error_e::unsupported, fmt::format("{}: {}", path, number));
        }
        return json_t(number);
    }

    [[nodiscard]] auto to_json(const dyn_value::seq_t& seq, const dyn_path& path) -> expected<json_t, error> {
        return collect<std::vector<json_t>>(
            seq,
            [&path](const dyn_value& item, std::size_t index) {
                return to_json(item, path / index);
            }
        )
            .transform([](std::vector<json_t> items) {
                return json_t(std::move(items)); }
            );
    }

    [[nodiscard]] auto to_json(const dyn_value::map_t& map, const dyn_path& path) -> expected<json_t, error> {
        const auto entry = [&path](const auto& item) {
            return to_json(item.second, path / item.first)
                .transform(
                    [&item](json_t value) {
                        return std::pair{ item.first, std::move(value) };
                    }
                );
        };

        return collect<std::map<std::string, json_t>>(map, entry)
            .transform(
                [](std::map<std::string, json_t> items) {
                    return json_t(std::move(items));
                }
            );
    }

    /**
     * @brief   A bool, an integer or a string, as is.
     */
    template <typename T>
    [[nodiscard]] auto to_json(const T& scalar, const dyn_path& /* path */) -> expected<json_t, error> {
        return json_t(scalar);
    }

    [[nodiscard]] auto to_json(const dyn_value& node, const dyn_path& path) -> expected<json_t, error> {
        return std::visit(
            [&path](const auto& contained) {
                return to_json(contained, path);
            }
            , node.data()
        );
    }

} // namespace

auto parse(std::string_view text) -> expected<dyn_value, error> {
    try {
        return from_json(json_t::parse(text));
    } catch (const json_t::exception& ex) {
        return wrap_error(dyn_value::error_e::syntax, ex.what());
    }
}

auto format(const dyn_value& doc) -> expected<std::string, error> {
    constexpr auto indent = 4;
    return to_json(doc, { })
        .transform(
            [](const json_t& json) {
                return fmt::format("{}\n", json.dump(indent));
            }
        );
}

} // namespace nova::detail::json
