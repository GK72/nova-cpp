/**
 * Part of Nova C++ Library
 *
 * @brief   Dynamically typed value trees: a YAML, JSON or TOML document in a
 *          format-agnostic form.
 *
 * Lookups are strict. A value of the wrong type throws, even when a fallback
 * is given; only a missing value falls back. A null counts as missing, and in
 * `merge` it deletes the key, so a single document reads the same as that
 * document merged onto nothing.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-10-31
 * @updated     2026-10-31
 * @status      Experimental
 */

#pragma once

#include <libnova/error.hpp>
#include <libnova/expected.hpp>

#include <fmt/format.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <initializer_list>
#include <limits>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace nova {

/**
 * @brief   A location in a `dyn_value` tree: map keys, and indices into
 *          sequences.
 *
 * - From a dotted string: `"cmake.definitions"`; `\.` escapes a dot in a key.
 * - From segments, taken verbatim: `{ "files", "foo.cpp" }`. A braced single
 *   string is one segment too: `dyn_path{ "a.b" }` is the key `a.b`.
 * - By appending: `path / "key"`, `path / 0`.
 *
 * The empty path is the root. A segment indexes a sequence if it is a number.
 */
class dyn_path {
public:
    dyn_path() = default;

    dyn_path(std::string_view dotted);

    dyn_path(const char* dotted)
        : dyn_path(std::string_view{ dotted })
    {}

    dyn_path(const std::string& dotted)
        : dyn_path(std::string_view{ dotted })
    {}

    dyn_path(std::initializer_list<std::string_view> segments);

    [[nodiscard]] auto segments() const -> const std::vector<std::string>& {
        return m_segments;
    }

    [[nodiscard]] auto empty() const -> bool {
        return m_segments.empty();
    }

    /**
     * @brief   This path and one more key, taken verbatim: `path / "foo.cpp"`
     *          is a single key.
     */
    [[nodiscard]] auto operator/(std::string_view key) const -> dyn_path;

    [[nodiscard]] auto operator/(const char* key) const -> dyn_path {
        return *this / std::string_view{ key };
    }

    [[nodiscard]] auto operator/(const std::string& key) const -> dyn_path {
        return *this / std::string_view{ key };
    }

    [[nodiscard]] auto operator/(std::size_t index) const -> dyn_path;

    /**
     * @brief   This path followed by `tail`.
     */
    [[nodiscard]] auto operator/(const dyn_path& tail) const -> dyn_path;

    /**
     * @brief   The first `count` segments.
     */
    [[nodiscard]] auto prefix(std::size_t count) const -> dyn_path;

    /**
     * @brief   The dotted form, dots in keys escaped; `<root>` if empty.
     */
    [[nodiscard]] auto to_string() const -> std::string;

    auto operator==(const dyn_path&) const -> bool = default;

private:
    std::vector<std::string> m_segments;

};

/**
 * @brief   Why a `dyn_value` is not the type asked for.
 */
struct dyn_mismatch {
    dyn_path where;             ///< Relative to the converted value
    std::string expected;
    std::string got;
};

} // namespace nova

template <>
struct fmt::formatter<nova::dyn_path> : fmt::formatter<std::string_view> {
    auto format(const nova::dyn_path& path, format_context& ctx) const {
        return fmt::formatter<std::string_view>::format(path.to_string(), ctx);
    }
};

template <>
struct fmt::formatter<nova::dyn_mismatch> : fmt::formatter<std::string_view> {
    /**
     * @format  `where`: expected X, got Y
     */
    auto format(const nova::dyn_mismatch& mismatch, format_context& ctx) const {
        return fmt::format_to(ctx.out(), "`{}`: expected {}, got {}", mismatch.where, mismatch.expected, mismatch.got);
    }
};

namespace nova {

class dyn_value;

/**
 * @brief   Conversion of a `dyn_value` into a `T`, the extension point of
 *          `dyn_value::lookup`.
 *
 * Specializations provide
 * - `static auto name() -> std::string`: what a `T` is called in errors;
 * - `static auto convert(const dyn_value&) -> expected<T, dyn_mismatch>`.
 *
 * `convert` never sees a null at the top, since a null is missing; a null
 * inside a sequence is an element like any other.
 */
template <typename T>
struct dyn_convert;

template <typename T>
concept dyn_convertible_c = requires (const dyn_value& node) {
    { dyn_convert<T>::name() } -> std::convertible_to<std::string>;
    { dyn_convert<T>::convert(node) } -> std::same_as<expected<T, dyn_mismatch>>;
};

/**
 * @brief   A dynamically typed value: null, bool, integer, floating point,
 *          string, sequence or map.
 */
class dyn_value {
public:
    using seq_t = std::vector<dyn_value>;
    using map_t = std::map<std::string, dyn_value, std::less<>>;
    using data_t = std::variant<std::monostate, bool, std::int64_t, double, std::string, seq_t, map_t>;

    /**
     * @brief   The formats `parse` and `read_from` understand.
     */
    enum class format_e : std::uint8_t {
        json,
        toml,
        yaml,
    };

    /**
     * @brief   Failures of reading a document, in the `dyn_value` category.
     *
     * A missing file is `std::errc::no_such_file_or_directory`.
     */
    enum class error_e : std::uint8_t {
        unknown_format = 1,     ///< The detail has the path
        syntax,                 ///< The detail has the path and the position
        not_a_map,              ///< A config whose top level is not a map
        unsupported,            ///< No counterpart in the format; the detail has the location
    };

    /**
     * @brief   Null.
     */
    dyn_value() = default;

    dyn_value(std::nullptr_t) {}

    dyn_value(bool flag)
        : m_data(flag)
    {}

    template <std::integral T>
        requires (not std::same_as<T, bool>)
    dyn_value(T number)
        : m_data(static_cast<std::int64_t>(number))
    {}

    dyn_value(double number)
        : m_data(number)
    {}

    /**
     * @brief   The `double` of the shortest decimal form of `number`: `0.1F`
     *          becomes 0.1, not 0.10000000149011612, so a written document
     *          shows it as it was typed.
     */
    dyn_value(float number);

    dyn_value(const char* text)
        : m_data(std::string(text))
    {}

    dyn_value(std::string_view text)
        : m_data(std::string(text))
    {}

    dyn_value(std::string text)
        : m_data(std::move(text))
    {}

    dyn_value(seq_t seq)
        : m_data(std::move(seq))
    {}

    dyn_value(map_t map)
        : m_data(std::move(map))
    {}

    [[nodiscard]] static auto seq(std::initializer_list<dyn_value> items) -> dyn_value;
    [[nodiscard]] static auto map(std::initializer_list<std::pair<const std::string, dyn_value>> items) -> dyn_value;

    /**
     * @brief   The value at `path` as a `T`.
     *
     * @throws  if it is missing, null, or not a `T`.
     */
    template <dyn_convertible_c T>
    [[nodiscard]] auto lookup(const dyn_path& path = { }) const -> T;

    /**
     * @brief   The value at `path` as a `T`; `fallback` if missing or null.
     *
     * @throws  if it is not a `T`.
     */
    template <dyn_convertible_c T>
    [[nodiscard]] auto lookup(const dyn_path& path, T fallback) const -> T;

    /**
     * @brief   The value at `path` as a `T`
     *
     * For an optional setting without a sensible default, or a default that
     * is expensive to compute.
     *
     * @returns empty if missing or null.
     *
     * @throws  if it is not a `T`.
     */
    template <dyn_convertible_c T>
    [[nodiscard]] auto find(const dyn_path& path = { }) const -> std::optional<T>;

    /**
     * @brief   Whether `lookup<T>(path)` would succeed.
     */
    template <dyn_convertible_c T>
    [[nodiscard]] auto is(const dyn_path& path = { }) const -> bool;

    [[nodiscard]] auto is_null() const -> bool {
        return std::holds_alternative<std::monostate>(m_data);
    }

    /**
     * @brief   The subtree at `path`, without copying it.
     *
     * @throws  if it is missing or null.
     */
    [[nodiscard]] auto at(const dyn_path& path) const -> const dyn_value&;

    /**
     * @throws  if not a map.
     */
    [[nodiscard]] auto as_map() const -> const map_t&;

    /**
     * @throws  if not a sequence.
     */
    [[nodiscard]] auto as_seq() const -> const seq_t&;

    [[nodiscard]] auto data() const -> const data_t& {
        return m_data;
    }

    /**
     * @brief   `null`, `bool`, `int`, `float`, `string`, `seq` or `map`.
     */
    [[nodiscard]] auto type_name() const -> std::string_view;

    auto operator==(const dyn_value&) const -> bool = default;

    /**
     * @brief   `base` with `overlay` on top.
     *
     * Maps merge key by key, recursively. A null in an `overlay` map deletes
     * the key. Anything else, a sequence too, replaces what `base` has. The
     * result has no nulls in its maps.
     */
    [[nodiscard]] static auto merge(dyn_value base, const dyn_value& overlay) -> dyn_value;

    /**
     * @brief   The format of `path`, by its extension.
     */
    [[nodiscard]] static auto format_of(const std::filesystem::path& path) -> expected<format_e, error>;

    /**
     * @brief   The document in `text`
     *
     * @returns null if `text` is blank, whatever the format.
     */
    [[nodiscard]] static auto parse(std::string_view text, format_e format) -> expected<dyn_value, error>;

    /**
     * @brief   The document in the file at `path`, in the format of its
     *          extension
     *
     * @returns null if the file is blank.
     */
    [[nodiscard]] static auto read_from(const std::filesystem::path& path) -> expected<dyn_value, error>;

    /**
     * @brief   `doc` as a config: always a map.
     *
     * @returns an empty map for a null (a blank document, or one with only
     *          comments); an error if `doc` is anything but a map.
     */
    [[nodiscard]] static auto as_config(dyn_value doc) -> expected<dyn_value, error>;

    /**
     * @brief   The config in the file at `path`, see `as_config`.
     *
     * A missing file is an error.
     */
    [[nodiscard]] static auto read_config(const std::filesystem::path& path) -> expected<dyn_value, error>;

    /**
     * @brief   The document text in `format`, ending with a new line.
     *
     * Reads back as the same tree, except for what the format cannot hold:
     * TOML has no null and needs a map at the top level, JSON has no
     * infinity or NaN; those are `error_e::unsupported`.
     */
    [[nodiscard]] auto dump(format_e format) const -> expected<std::string, error>;

    /**
     * @brief   Write the document to `path`, in the format of its extension,
     *          creating its directory.
     *
     * The file is replaced at once: a failed write leaves the old one in place.
     */
    [[nodiscard]] auto write_to(const std::filesystem::path& path) const -> expected<empty, error>;

private:
    data_t m_data;

    /**
     * @brief   The node at `path`, or `nullptr` if a key is missing, an index
     *          is out of range, or the node is null.
     *
     * @returns a mismatch if `path` cannot be followed: it continues past a
     *          scalar (`jobs.deeper` with `jobs: 8`), or uses a key on a
     *          sequence.
     */
    [[nodiscard]] auto walk(const dyn_path& path) const -> expected<const dyn_value*, dyn_mismatch>;

    /**
     * @brief   Throws `mismatch`, its location relative to `base`.
     */
    [[noreturn]] static void throw_mismatch(const dyn_path& base, const dyn_mismatch& mismatch);
};

} // namespace nova

template <>
struct nova::error_code_traits<nova::dyn_value::error_e> {
    static constexpr auto name = "dyn_value";
    static constexpr auto messages = std::to_array<std::string_view>({
        "Unknown format",
        "Syntax error",
        "Not a map",
        "Unsupported value",
    });

    static_assert(messages.size() == static_cast<std::size_t>(nova::dyn_value::error_e::unsupported));
};

/**
 * @brief   The text of a scalar.
 *
 * Floating point numbers print in their shortest form: `1.10` as `1.1`.
 *
 * @throws  for a null, a sequence or a map.
 */
template <>
struct fmt::formatter<nova::dyn_value> : fmt::formatter<std::string_view> {
    auto format(const nova::dyn_value& node, format_context& ctx) const -> format_context::iterator;
};

namespace nova {

// --- Conversions ---

namespace detail {

    template <typename T>
    [[nodiscard]] auto dyn_fail(const dyn_value& node) -> expected<T, dyn_mismatch> {
        return unexpected<dyn_mismatch>{ {
            .where = { },
            .expected = dyn_convert<T>::name(),
            .got = std::string(node.type_name()),
        } };
    }

} // namespace detail

template <>
struct dyn_convert<bool> {
    [[nodiscard]] static auto name() -> std::string { return "bool"; }

    [[nodiscard]] static auto convert(const dyn_value& node) -> expected<bool, dyn_mismatch> {
        if (const auto* flag = std::get_if<bool>(&node.data())) {
            return *flag;
        }
        return detail::dyn_fail<bool>(node);
    }
};

/**
 * @brief   Integers, if in range; a floating point number is never an
 *          integer.
 */
template <std::integral T>
    requires (not std::same_as<T, bool>)
struct dyn_convert<T> {
    [[nodiscard]] static auto name() -> std::string {
        return fmt::format("{}int{}", std::is_signed_v<T> ? "" : "u", std::numeric_limits<std::make_unsigned_t<T>>::digits);
    }

    [[nodiscard]] static auto convert(const dyn_value& node) -> expected<T, dyn_mismatch> {
        const auto* number = std::get_if<std::int64_t>(&node.data());

        if (number == nullptr) {
            return detail::dyn_fail<T>(node);
        }

        if (not std::in_range<T>(*number)) {
            return unexpected<dyn_mismatch>{ {
                .where = { },
                .expected = name(),
                .got = fmt::format("{} (out of range)", *number),
            } };
        }

        return static_cast<T>(*number);
    }
};

/**
 * @brief   Floating point numbers, integers too.
 */
template <std::floating_point T>
struct dyn_convert<T> {
    [[nodiscard]] static auto name() -> std::string {
        return "float";
    }

    [[nodiscard]] static auto convert(const dyn_value& node) -> expected<T, dyn_mismatch> {
        if (const auto* number = std::get_if<double>(&node.data())) {
            return static_cast<T>(*number);
        }

        if (const auto* number = std::get_if<std::int64_t>(&node.data())) {
            return static_cast<T>(*number);
        }

        return detail::dyn_fail<T>(node);
    }
};

/**
 * @brief   Strings only.
 *
 * Format a scalar for its text (`fmt::format("{}", doc.at(path))`).
 */
template <>
struct dyn_convert<std::string> {
    [[nodiscard]] static auto name() -> std::string {
        return "string";
    }

    [[nodiscard]] static auto convert(const dyn_value& node) -> expected<std::string, dyn_mismatch> {
        if (const auto* text = std::get_if<std::string>(&node.data())) {
            return *text;
        }
        return detail::dyn_fail<std::string>(node);
    }
};

/**
 * @brief   Any value, as a copy.
 *
 * `dyn_value::at` gives a reference.
 */
template <>
struct dyn_convert<dyn_value> {
    [[nodiscard]] static auto name() -> std::string {
        return "value";
    }

    [[nodiscard]] static auto convert(const dyn_value& node) -> expected<dyn_value, dyn_mismatch> {
        return node;
    }
};

template <dyn_convertible_c T, typename Alloc>
struct dyn_convert<std::vector<T, Alloc>> {
    using R = std::vector<T, Alloc>;

    [[nodiscard]] static auto name() -> std::string {
        return fmt::format("seq of {}", dyn_convert<T>::name());
    }

    [[nodiscard]] static auto convert(const dyn_value& node) -> expected<R, dyn_mismatch> {
        const auto* seq = std::get_if<dyn_value::seq_t>(&node.data());

        if (seq == nullptr) {
            return detail::dyn_fail<R>(node);
        }

        auto ret = R{ };
        ret.reserve(seq->size());

        for (std::size_t index = 0; const auto& value : *seq) {
            auto item = dyn_convert<T>::convert(value);

            if (not item) {
                auto mismatch = item.error();
                mismatch.where = dyn_path{ } / index / mismatch.where;
                return unexpected<dyn_mismatch>{ std::move(mismatch) };
            }

            ret.push_back(std::move(*item));
            ++index;
        }

        return ret;
    }
};

/**
 * @brief   Maps with string keys; null values are left out, as missing.
 */
template <dyn_convertible_c T, typename Compare, typename Alloc>
struct dyn_convert<std::map<std::string, T, Compare, Alloc>> {
    using R = std::map<std::string, T, Compare, Alloc>;

    [[nodiscard]] static auto name() -> std::string {
        return fmt::format("map of {}", dyn_convert<T>::name());
    }

    [[nodiscard]] static auto convert(const dyn_value& node) -> expected<R, dyn_mismatch> {
        const auto* map = std::get_if<dyn_value::map_t>(&node.data());
        if (map == nullptr) {
            return detail::dyn_fail<R>(node);
        }

        auto ret = R{ };

        for (const auto& [key, value] : *map) {
            if (value.is_null()) {
                continue;
            }

            auto item = dyn_convert<T>::convert(value);

            if (not item) {
                auto mismatch = item.error();
                mismatch.where = dyn_path{ key } / mismatch.where;
                return unexpected<dyn_mismatch>{ std::move(mismatch) };
            }

            ret.emplace(key, std::move(*item));
        }

        return ret;
    }
};

// --- Lookup ---

template <dyn_convertible_c T>
auto dyn_value::find(const dyn_path& path) const -> std::optional<T> {
    const auto node = walk(path);

    if (not node) {
        throw_mismatch({ }, node.error());
    }

    if (*node == nullptr) {
        return std::nullopt;
    }

    auto ret = dyn_convert<T>::convert(**node);

    if (not ret) {
        throw_mismatch(path, ret.error());
    }

    return std::move(*ret);
}

template <dyn_convertible_c T>
auto dyn_value::lookup(const dyn_path& path) const -> T {
    auto ret = find<T>(path);

    if (not ret) {
        throw exception("Missing `{}`", path);
    }

    return std::move(*ret);
}

template <dyn_convertible_c T>
auto dyn_value::lookup(const dyn_path& path, T fallback) const -> T {
    auto ret = find<T>(path);
    return ret ? std::move(*ret) : std::move(fallback);
}

template <dyn_convertible_c T>
auto dyn_value::is(const dyn_path& path) const -> bool {
    const auto node = walk(path);
    return node and *node != nullptr and dyn_convert<T>::convert(**node).has_value();
}

} // namespace nova
