/**
 * Part of Nova C++ Library
 *
 * Generic attribute tree abstraction, meant as a common target for
 * config file readers (JSON, YAML, XML, ...) to parse into.
 *
 * The reflection-based binder that maps a tree onto typed structs lives
 * in `value_refl.hpp`, gated behind `NOVA_EXPERIMENTAL_FEATURE_SET`; this
 * header itself does not require reflection.
 *
 * @author  Gábor Krisztián Girhiny
 * @date    2026-10-03
 */

#pragma once

#include <libnova/error.hpp>
#include <libnova/utils.hpp>

#include <fmt/format.h>

#include <concepts>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#ifdef NOVA_EXPERIMENTAL_FEATURE_SET
#include <meta>
#endif

namespace nova {

template <typename Traits>
class basic_value;

struct default_value_traits {
    template <typename Traits>
    using seq = std::vector<basic_value<Traits>>;

    template <typename Traits>
    using map = std::map<std::string, basic_value<Traits>, std::less<>>;

    using string = std::string;

    using ext = std::tuple<>;
};

namespace detail {

/**
 * @brief   The variant shape backing `basic_value<Traits>`, and the sole authority on
 *          naming its alternatives for diagnostics.
 *
 * Bundled together because they're the same concern: `Traits::ext` is an extension
 * point on the alternatives, so whatever defines them also owns naming them, rather
 * than `basic_value` hardcoding names for a set of alternatives it doesn't itself pick.
 */
template <typename Traits>
struct value_union {
    using seq_t = typename Traits::template seq<Traits>;
    using map_t = typename Traits::template map<Traits>;
    using string_t = typename Traits::string;

    using type = variant_cat_t<
        typename Traits::ext,
        std::monostate, bool, std::int64_t, double, string_t, seq_t, map_t
    >;

    /**
     * @brief   Diagnosiable name of type `T`.
     *
     * Falls back to the compiler's spelling for types (such as fundamental types)
     * that have no reflectable identifier.
     */
    template <typename T>
    static consteval auto name_of() -> std::string_view {
        if constexpr (std::same_as<T, std::monostate>) {
            return "null";
        } else if constexpr (std::same_as<T, bool>) {
            return "bool";
        } else if constexpr (std::same_as<T, std::int64_t>) {
            return "int64";
        } else if constexpr (std::same_as<T, std::string>) {
            return "string";
        } else if constexpr (std::same_as<T, seq_t>) {
            return "seq";
        } else if constexpr (std::same_as<T, map_t>) {
            return "map";
        } else {
#ifdef NOVA_EXPERIMENTAL_FEATURE_SET
            if constexpr (std::meta::has_identifier(^^T)) {
                return std::meta::identifier_of(^^T);
            } else {
                return std::meta::display_string_of(^^T);
            }
#else
            return "(unknown)";
#endif
        }
    }
};

template <typename Ret, typename Variant>
struct is_variant_alternative;

template <typename Ret, typename... Ts>
struct is_variant_alternative<Ret, std::variant<Ts...>> : std::disjunction<std::is_same<Ret, Ts>...> {};

template <typename Ret, typename Variant>
inline constexpr bool is_variant_alternative_v = is_variant_alternative<Ret, Variant>::value;

} // namespace detail

template <typename Traits = default_value_traits>
class basic_value {
public:
    using union_t = detail::value_union<Traits>;
    using seq_t = typename union_t::seq_t;
    using map_t = typename union_t::map_t;
    using data_t = typename union_t::type;

    basic_value() = default;

    basic_value(const char* s)
        : m_data(std::string(s))
    {}

    basic_value(int i)
        : m_data(static_cast<std::int64_t>(i))
    {}

    template <typename U>
        requires (
            not std::same_as<std::remove_cvref_t<U>, basic_value>
            && std::constructible_from<data_t, U&&>
        )
    basic_value(U&& u)
        : m_data(std::forward<U>(u))
    {}

    /**
     * @brief   Builds a sequence node.
     */
    static auto seq(std::initializer_list<basic_value> il) -> basic_value {
        return basic_value(seq_t(il.begin(), il.end()));
    }

    /**
     * @brief   Builds a map node.
     */
    static auto map(std::initializer_list<std::pair<std::string, basic_value>> il) -> basic_value {
        return basic_value(map_t(il.begin(), il.end()));
    }

    [[nodiscard]] auto data() const -> const data_t& {
        return m_data;
    }

    template <typename T>
    [[nodiscard]] auto is() const -> bool {
        return std::holds_alternative<T>(m_data);
    }

    /**
     * @brief   Extracts the value as a `Ret`.
     *
     * @returns by value for simple types else by const references
     *
     * Numeric types converts between each other.
     */
    template <typename Ret>
    [[nodiscard]] auto as() const -> std::conditional_t<std::is_class_v<Ret>, const Ret&, Ret> {
        if constexpr (std::is_class_v<Ret> && detail::is_variant_alternative_v<Ret, data_t>) {
            if (auto* p = std::get_if<Ret>(&m_data)) {
                return *p;
            }
            throw exception("Cannot convert {} to {}", type_name(), union_t::template name_of<Ret>());
        } else if constexpr (std::is_class_v<Ret>) {
            throw exception("Cannot convert {} to {}", type_name(), union_t::template name_of<Ret>());
        } else {
            return as_simple<Ret>();
        }
    }

    [[nodiscard]] auto as_map() const -> const map_t& {
        return as<map_t>();
    }

    [[nodiscard]] auto as_seq() const -> const seq_t& {
        return as<seq_t>();
    }

    [[nodiscard]] auto find(std::string_view key) const
            -> std::optional<std::reference_wrapper<const basic_value>>
    {
        auto* m = std::get_if<map_t>(&m_data);
        if (not m) {
            return std::nullopt;
        }

        auto it = m->find(key);
        if (it == m->end()) {
            return std::nullopt;
        }

        return std::cref(it->second);
    }

    [[nodiscard]] auto operator[](std::string_view key) const -> const basic_value& {
        if (auto v = find(key)) {
            return v->get();
        }
        throw exception("Missing key '{}'", key);
    }

    [[nodiscard]] auto operator[](std::size_t idx) const -> const basic_value& {
        const auto& s = as_seq();
        if (idx >= s.size()) {
            throw exception("Index {} out of range (size {})", idx, s.size());
        }
        return s[idx];
    }

    [[nodiscard]] auto type_name() const -> std::string_view {
        return std::visit(
            []<typename U>(const U&) {
                return union_t::template name_of<U>();
            }
            , m_data
        );
    }

private:
    data_t m_data;

    template <typename Ret>
    [[nodiscard]] auto as_simple() const -> Ret {
        std::optional<Ret> out;

        std::visit(
            [&out]<typename C>(const C& contained) {
                if constexpr (std::same_as<C, Ret>) {
                    out = contained;
                } else if constexpr (
                    (std::integral<Ret> || std::floating_point<Ret>)
                    && (std::same_as<C, std::int64_t> || std::same_as<C, double>)
                ) {
                    out = static_cast<Ret>(contained);
                }
            }
            , m_data
        );

        if (not out) {
            throw exception("Cannot convert {} to {}", type_name(), union_t::template name_of<Ret>());
        }

        return *out;
    }

};

using value = basic_value<default_value_traits>;

} // namespace nova
