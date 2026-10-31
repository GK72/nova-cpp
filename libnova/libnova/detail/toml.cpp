/**
 * Part of Nova C++ Library
 *
 * @brief   TOML reader and writer.
 *
 * toml++ does the reading: parsing, with syntax errors and their positions,
 * and typed values, dates and times too.
 *
 * Special handling is implemented for:
 * - Rejecting dates and times when reading: a `dyn_value` has no such type,
 *   neither do JSON or YAML 1.2. Reading them as strings would be possible,
 *   but writing them back would change their type in the file.
 * - All of the writing. toml++ has a writer (`toml_formatter`), but its floats
 *   are either full precision (`0.10000000000000001`) or, with
 *   `relaxed_float_precision`, lossy (`0.123456789` as `0.123457`). Ours
 *   writes the shortest exact form, always with a fraction or an exponent so
 *   that `2.0` stays a float.
 * - Writing keys bare where TOML allows it, else quoted, and strings as basic
 *   strings with escapes.
 * - Writing a table's values before its sub-tables, as TOML requires, the
 *   sub-tables as `[dotted.sections]`; arrays and tables within arrays stay
 *   inline (`[ { a = 1 } ]`).
 * - Rejecting what TOML cannot hold: a null anywhere, or a document that is
 *   not a map.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-10-23
 * @updated     2026-10-31
 * @status      Prototype
 */

#include <libnova/detail/toml.hpp>
#include <libnova/detail/collect.hpp>

#include <libnova/dyn_value.hpp>
#include <libnova/error.hpp>
#include <libnova/expected.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <toml++/toml.hpp>

#include <algorithm>
#include <array>
#include <charconv>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <variant>

namespace nova::detail::toml {

namespace {

    using map_t = dyn_value::map_t;
    using seq_t = dyn_value::seq_t;

    [[nodiscard]] auto unsupported(const dyn_path& path) -> unexpected<error> {
        return wrap_error(dyn_value::error_e::unsupported, path.to_string());
    }

    template <typename T>
    concept scalar_c = std::same_as<T, std::string>
        or std::same_as<T, std::int64_t>
        or std::same_as<T, double>
        or std::same_as<T, bool>;

    [[nodiscard]] auto from_node(const ::toml::node& node, const dyn_path& path) -> expected<dyn_value, error>;

    [[nodiscard]] auto from_typed(const ::toml::table& table, const dyn_path& path) -> expected<dyn_value, error> {
        const auto entry = [&path](const auto& item) {
            const auto key = std::string(item.first.str());
            return from_node(item.second, path / key)
                .transform([&key](dyn_value value) { return std::pair{ key, std::move(value) }; });
        };

        return collect<map_t>(table, entry)
            .transform([](map_t map) { return dyn_value(std::move(map)); });
    }

    [[nodiscard]] auto from_typed(const ::toml::array& array, const dyn_path& path) -> expected<dyn_value, error> {
        return collect<seq_t>(
            array,
            [&path](const ::toml::node& item, std::size_t index) {
                return from_node(item, path / index);
            }
        )
            .transform([](seq_t seq) { return dyn_value(std::move(seq)); });
    }

    /**
     * @brief   A string, an integer, a float or a bool; dates and times have
     *          no counterpart.
     */
    template <typename T>
    [[nodiscard]] auto from_typed(const ::toml::value<T>& value, const dyn_path& path) -> expected<dyn_value, error> {
        if constexpr (scalar_c<T>) {
            return dyn_value(value.get());
        } else {
            return unsupported(path);
        }
    }

    [[nodiscard]] auto from_node(const ::toml::node& node, const dyn_path& path) -> expected<dyn_value, error> {
        return node.visit([&path](const auto& typed) { return from_typed(typed, path); });
    }

    [[nodiscard]] auto is_bare_key(std::string_view key) -> bool {
        return not key.empty() and std::ranges::all_of(
            key,
            [](char chr) {
                return (chr >= 'a' and chr <= 'z')
                    or (chr >= 'A' and chr <= 'Z')
                    or (chr >= '0' and chr <= '9')
                    or chr == '_' or chr == '-';
            }
        );
    }

    [[nodiscard]] auto escaped(char chr) -> std::string {
        constexpr auto first_printable = 0x20;
        constexpr auto del = 0x7F;

        switch (chr) {
            case '"':  return "\\\"";
            case '\\': return "\\\\";
            case '\n': return "\\n";
            case '\t': return "\\t";
            case '\r': return "\\r";
            default:   break;
        }

        const auto code = static_cast<unsigned char>(chr);
        return code < first_printable or code == del ? fmt::format("\\u{:04X}", code) : std::string(1, chr);
    }

    [[nodiscard]] auto quoted_text(std::string_view text) -> std::string {
        return fmt::format("\"{}\"", fmt::join(text | std::views::transform(escaped), ""));
    }

    [[nodiscard]] auto key_text(std::string_view key) -> std::string {
        return is_bare_key(key) ? std::string(key) : quoted_text(key);
    }

    /**
     * @brief   The shortest form that reads back as `number`, always with a
     *          fraction or an exponent.
     */
    [[nodiscard]] auto float_text(double number) -> std::string {
        constexpr auto max_length = 32;

        auto buffer = std::array<char, max_length>{ };
        auto* end = std::to_chars(buffer.data(), buffer.data() + buffer.size(), number).ptr;
        const auto text = std::string(buffer.data(), end);
        return text.find_first_of(".ein") == std::string::npos ? text + ".0" : text;
    }

    /**
     * @brief   `items` between `open` and `close`, separated by commas.
     */
    [[nodiscard]] auto bracketed(const std::vector<std::string>& items, std::string_view open, std::string_view close) -> std::string {
        return items.empty()
            ? fmt::format("{}{}", open, close)
            : fmt::format("{} {} {}", open, fmt::join(items, ", "), close);
    }

    [[nodiscard]] auto inline_text(const dyn_value& node, const dyn_path& path) -> expected<std::string, error>;

    [[nodiscard]] auto inline_text(std::monostate /* null */, const dyn_path& path) -> expected<std::string, error> {
        return unsupported(path);
    }

    [[nodiscard]] auto inline_text(bool flag, const dyn_path& /* path */) -> expected<std::string, error> {
        return std::string(flag ? "true" : "false");
    }

    [[nodiscard]] auto inline_text(std::int64_t number, const dyn_path& /* path */) -> expected<std::string, error> {
        return fmt::format("{}", number);
    }

    [[nodiscard]] auto inline_text(double number, const dyn_path& /* path */) -> expected<std::string, error> {
        return float_text(number);
    }

    [[nodiscard]] auto inline_text(const std::string& text, const dyn_path& /* path */) -> expected<std::string, error> {
        return quoted_text(text);
    }

    [[nodiscard]] auto inline_text(const seq_t& seq, const dyn_path& path) -> expected<std::string, error> {
        return collect<std::vector<std::string>>(
            seq,
            [&path](const dyn_value& item, std::size_t index) {
                return inline_text(item, path / index);
            }
        )
            .transform([](const std::vector<std::string>& items) { return bracketed(items, "[", "]"); });
    }

    /**
     * @brief   `key = value`.
     */
    [[nodiscard]] auto entry_text(const auto& entry, const dyn_path& path) -> expected<std::string, error> {
        return inline_text(entry.second, path / entry.first)
            .transform(
                [&entry](const std::string& value) {
                    return fmt::format("{} = {}", key_text(entry.first), value);
                }
            );
    }

    [[nodiscard]] auto inline_text(const map_t& map, const dyn_path& path) -> expected<std::string, error> {
        return collect<std::vector<std::string>>(
            map,
            [&path](const auto& entry) {
                return entry_text(entry, path);
            }
        )
            .transform([](const std::vector<std::string>& items) { return bracketed(items, "{", "}"); });
    }

    /**
     * @brief   `node` as an inline value: a scalar, an array or an inline table.
     */
    [[nodiscard]] auto inline_text(const dyn_value& node, const dyn_path& path) -> expected<std::string, error> {
        return std::visit(
            [&path](const auto& contained) {
                return inline_text(contained, path);
            }
            , node.data()
        );
    }

    [[nodiscard]] auto is_table(const auto& entry) -> bool {
        return std::holds_alternative<map_t>(entry.second.data());
    }

    [[nodiscard]] auto table_text(const map_t& table, const dyn_path& path, const std::string& header) -> expected<std::string, error>;

    /**
     * @brief   The section of the table under `entry`, after a blank line.
     */
    [[nodiscard]] auto section_text(const auto& entry, const dyn_path& path, const std::string& header) -> expected<std::string, error> {
        const auto section = header.empty()
            ? key_text(entry.first)
            : fmt::format("{}.{}", header, key_text(entry.first));

        return table_text(entry.second.as_map(), path / entry.first, section)
            .transform([&section](const std::string& body) { return fmt::format("\n[{}]\n{}", section, body); });
    }

    /**
     * @brief   The entries of the table at `path`: its values, then its tables
     *          as sections.
     *
     * @param   header  the dotted, quoted where needed, name of the table's section
     */
    [[nodiscard]] auto table_text(const map_t& table, const dyn_path& path, const std::string& header) -> expected<std::string, error> {
        auto values = table | std::views::filter([](const auto& entry) { return not is_table(entry); });
        auto tables = table | std::views::filter([](const auto& entry) { return is_table(entry); });

        return collect<std::vector<std::string>>(values, [&path](const auto& entry) { return entry_text(entry, path); })
            .and_then([&](const std::vector<std::string>& lines) {
                return collect<std::vector<std::string>>(tables, [&](const auto& entry) { return section_text(entry, path, header); })
                    .transform([&lines](const std::vector<std::string>& sections) {
                        return fmt::format(
                            "{}{}",
                            fmt::join(lines | std::views::transform([](const std::string& line) { return line + '\n'; }), ""),
                            fmt::join(sections, "")
                        );
                    });
            });
    }

    [[nodiscard]] auto document_text(const map_t& table) -> expected<std::string, error> {
        return table_text(table, { }, "").transform([](const std::string& text) {
            return text.starts_with('\n') ? text.substr(1) : text;
        });
    }

} // namespace

auto parse(std::string_view text) -> expected<dyn_value, error> {
    try {
        return from_node(::toml::parse(text), { });
    } catch (const ::toml::parse_error& ex) {
        const auto& at = ex.source().begin;
        return wrap_error(dyn_value::error_e::syntax, fmt::format("{}:{}: {}", at.line, at.column, ex.description()));
    }
}

auto format(const dyn_value& doc) -> expected<std::string, error> {
    const auto* table = std::get_if<map_t>(&doc.data());
    return table != nullptr
        ? document_text(*table)
        : expected<std::string, error>{ unsupported({ }) };
}

} // namespace nova::detail::toml
