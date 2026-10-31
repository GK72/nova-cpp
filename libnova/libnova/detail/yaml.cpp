/**
 * Part of Nova C++ Library
 *
 * @brief   YAML reader and writer.
 *
 * yaml-cpp does:
 * - parsing text into a node tree, with syntax errors and their positions;
 * - telling quoted scalars from plain ones (by their tag);
 * - writing: block layout, indentation, escaping, and quoting strings that
 *   are not valid plain scalars (`a: b`).
 *
 * Special handling is implemented for:
 * - Resolving plain scalars by the YAML 1.2 core schema (null, bool, integer
 *   with `0x`/`0o`, float with `.inf`/`.nan`, else string). yaml-cpp keeps
 *   every scalar as text, and its conversions follow YAML 1.1, where `on` and
 *   `yes` are booleans; a `dyn_value` needs the type decided at parsing, and
 *   by the current standard. An integer beyond `std::int64_t` is a float.
 * - Rejecting map keys that are not scalars: keys are strings in a
 *   `dyn_value`.
 * - Quoting strings that would read back as another type (`"42"`, `"true"`,
 *   `"1.10"`, `"~"`): yaml-cpp writes them plain, so the round trip would
 *   change their type.
 * - Writing floats in their shortest exact form, always with a fraction or an
 *   exponent: yaml-cpp writes `2.0` as `2`, which reads back as an integer.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-10-31
 * @updated     2026-10-31
 * @status      Prototype
 */

#include <libnova/detail/yaml.hpp>
#include <libnova/detail/collect.hpp>

#include <libnova/dyn_value.hpp>
#include <libnova/error.hpp>
#include <libnova/expected.hpp>

#include <fmt/format.h>
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>

namespace nova::detail::yaml {

namespace {

    /**
     * @brief   `text` as an integer of the core schema, if all of it is one.
     */
    [[nodiscard]] auto yaml_int(std::string_view text) -> std::optional<std::int64_t> {
        auto base = 10;
        auto negative = false;

        if (text.starts_with("0x")) {
            base = 16;
            text.remove_prefix(2);
        } else if (text.starts_with("0o")) {
            base = 8;
            text.remove_prefix(2);
        } else if (text.starts_with('-') or text.starts_with('+')) {
            negative = text.front() == '-';
            text.remove_prefix(1);
        }

        if (text.empty()) {
            return std::nullopt;
        }

        std::uint64_t magnitude = 0;
        const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), magnitude, base);
        if (ec != std::errc{} or end != text.data() + text.size()) {
            return std::nullopt;
        }

        constexpr auto max = static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
        if (negative and magnitude <= max + 1) {
            return static_cast<std::int64_t>(0 - magnitude);
        }
        if (not negative and magnitude <= max) {
            return static_cast<std::int64_t>(magnitude);
        }
        return std::nullopt;
    }

    /**
     * @brief   `text` as a float of the core schema, if all of it is one.
     */
    [[nodiscard]] auto yaml_float(std::string_view text) -> std::optional<double> {
        static constexpr auto specials = std::to_array<std::pair<std::string_view, double>>({
            { ".inf",  std::numeric_limits<double>::infinity() },
            { ".Inf",  std::numeric_limits<double>::infinity() },
            { ".INF",  std::numeric_limits<double>::infinity() },
            { "+.inf", std::numeric_limits<double>::infinity() },
            { "+.Inf", std::numeric_limits<double>::infinity() },
            { "+.INF", std::numeric_limits<double>::infinity() },
            { "-.inf", -std::numeric_limits<double>::infinity() },
            { "-.Inf", -std::numeric_limits<double>::infinity() },
            { "-.INF", -std::numeric_limits<double>::infinity() },
            { ".nan",  std::numeric_limits<double>::quiet_NaN() },
            { ".NaN",  std::numeric_limits<double>::quiet_NaN() },
            { ".NAN",  std::numeric_limits<double>::quiet_NaN() },
        });

        for (const auto& [spelling, number] : specials) {
            if (text == spelling) {
                return number;
            }
        }

        auto digits = text;
        if (digits.starts_with('+') or digits.starts_with('-')) {
            digits.remove_prefix(1);
        }

        // `from_chars` also takes `inf`, `nan` and hex floats, none of which is a core schema float
        if (digits.empty() or not (digits.front() == '.' or (digits.front() >= '0' and digits.front() <= '9'))) {
            return std::nullopt;
        }

        if (digits.find_first_of("xXpP") != std::string_view::npos) {
            return std::nullopt;
        }

        // `from_chars` rejects a leading `+`
        if (text.starts_with('+')) {
            text.remove_prefix(1);
        }

        double number = 0;
        const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), number);
        if (ec != std::errc{} or end != text.data() + text.size()) {
            return std::nullopt;
        }

        return number;
    }

    [[nodiscard]] auto is_null(std::string_view text) -> bool {
        return text.empty() or text == "~" or text == "null" or text == "Null" or text == "NULL";
    }

    [[nodiscard]] auto as_bool(std::string_view text) -> std::optional<bool> {
        if (text == "true" or text == "True" or text == "TRUE") {
            return true;
        }
        if (text == "false" or text == "False" or text == "FALSE") {
            return false;
        }
        return std::nullopt;
    }

    /**
     * @brief   A plain scalar by the core schema.
     */
    [[nodiscard]] auto resolve(const std::string& text) -> dyn_value {
        if (is_null(text)) {
            return { };
        }
        if (const auto flag = as_bool(text)) {
            return *flag;
        }
        if (const auto number = yaml_int(text)) {
            return *number;
        }
        if (const auto number = yaml_float(text)) {
            return *number;
        }
        return text;
    }

    [[nodiscard]] auto from_node(const YAML::Node& node) -> expected<dyn_value, error>;

    /**
     * @brief   A scalar: quoted ones carry the non-specific tag `!` and are
     *          strings, plain ones `?` and resolve by the core schema.
     */
    [[nodiscard]] auto from_scalar(const YAML::Node& node) -> dyn_value {
        return node.Tag() == "?" ? resolve(node.Scalar()) : dyn_value(node.Scalar());
    }

    [[nodiscard]] auto from_seq(const YAML::Node& node) -> expected<dyn_value, error> {
        return collect<dyn_value::seq_t>(node, [](const YAML::Node& item) { return from_node(item); })
            .transform([](dyn_value::seq_t seq) { return dyn_value(std::move(seq)); });
    }

    /**
     * @brief   Keys must be scalars: they are strings in a `dyn_value`.
     */
    [[nodiscard]] auto key_of(const YAML::Node& key) -> expected<std::string, error> {
        if (not key.IsScalar()) {
            return wrap_error(dyn_value::error_e::unsupported, fmt::format("line {}: a map key that is not a scalar", key.Mark().line + 1));
        }
        return key.Scalar();
    }

    /**
     * @brief   The last of repeated keys wins.
     */
    [[nodiscard]] auto from_map(const YAML::Node& node) -> expected<dyn_value, error> {
        const auto entry = [](const auto& item) {
            return key_of(item.first).and_then([&item](std::string key) {
                return from_node(item.second).transform([&key](dyn_value value) { return std::pair{ std::move(key), std::move(value) }; });
            });
        };
        return collect<dyn_value::map_t>(node, entry).transform([](dyn_value::map_t map) { return dyn_value(std::move(map)); });
    }

    [[nodiscard]] auto from_node(const YAML::Node& node) -> expected<dyn_value, error> {
        switch (node.Type()) {
            case YAML::NodeType::Scalar:    return from_scalar(node);
            case YAML::NodeType::Sequence:  return from_seq(node);
            case YAML::NodeType::Map:       return from_map(node);
            case YAML::NodeType::Undefined:
            case YAML::NodeType::Null:
                break;
        }

        return dyn_value{ };
    }

    /**
     * @brief   Whether `text` written plain would read back as something else
     *          than this string.
     */
    [[nodiscard]] auto needs_quotes(const std::string& text) -> bool {
        return not std::holds_alternative<std::string>(resolve(text).data());
    }

    /**
     * @brief   The shortest form that reads back as `number`, as a float.
     */
    [[nodiscard]] auto float_text(double number) -> std::string {
        if (std::isnan(number)) {
            return ".nan";
        }
        if (std::isinf(number)) {
            return number > 0 ? ".inf" : "-.inf";
        }

        auto buffer = std::array<char, 32>{ };
        const auto end = std::to_chars(buffer.data(), buffer.data() + buffer.size(), number).ptr;
        auto ret = std::string(buffer.data(), end);
        if (ret.find_first_of(".e") == std::string::npos) {
            ret += ".0";
        }
        return ret;
    }

    void emit(YAML::Emitter& out, const dyn_value& node);

    void emit(YAML::Emitter& out, std::monostate /* null */) {
        out << YAML::Null;
    }

    void emit(YAML::Emitter& out, bool flag) {
        out << YAML::TrueFalseBool << flag;
    }

    void emit(YAML::Emitter& out, std::int64_t number) {
        out << number;
    }

    void emit(YAML::Emitter& out, double number) {
        out << float_text(number);
    }

    /**
     * @brief   Quoted if it would read back as something else plain.
     */
    void emit(YAML::Emitter& out, const std::string& text) {
        out << (needs_quotes(text) ? YAML::DoubleQuoted : YAML::Auto) << text;
    }

    void emit(YAML::Emitter& out, const dyn_value::seq_t& seq) {
        out << YAML::BeginSeq;
        std::ranges::for_each(seq, [&out](const dyn_value& item) { emit(out, item); });
        out << YAML::EndSeq;
    }

    void emit(YAML::Emitter& out, const dyn_value::map_t& map) {
        out << YAML::BeginMap;
        std::ranges::for_each(map, [&out](const auto& entry) {
            out << YAML::Key << entry.first << YAML::Value;
            emit(out, entry.second);
        });
        out << YAML::EndMap;
    }

    void emit(YAML::Emitter& out, const dyn_value& node) {
        std::visit([&out](const auto& contained) { emit(out, contained); }, node.data());
    }

} // namespace

auto parse(std::string_view text) -> expected<dyn_value, error> {
    try {
        return from_node(YAML::Load(std::string(text)));
    } catch (const YAML::Exception& ex) {
        return wrap_error(dyn_value::error_e::syntax, ex.what());
    }
}

auto format(const dyn_value& doc) -> expected<std::string, error> {
    auto out = YAML::Emitter{ };
    emit(out, doc);

    if (not out.good()) {
        return wrap_error(dyn_value::error_e::unsupported, out.GetLastError());
    }

    return fmt::format("{}\n", out.c_str());
}

} // namespace nova::detail::yaml
