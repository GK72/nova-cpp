/**
 * Part of Nova C++ Library
 *
 * Reflection-based binder mapping a `nova::basic_value` attribute tree onto
 * (and off of) typed aggregates, enums and sequences.
 *
 * Requires `NOVA_EXPERIMENTAL_FEATURE_SET` (reflection); this entire header
 * is a no-op without it.
 *
 * @author  Gábor Krisztián Girhiny
 * @date    2026-10-03
 */

#pragma once

#ifdef NOVA_EXPERIMENTAL_FEATURE_SET

#include <libnova/error.hpp>
#include <libnova/refl.hpp>
#include <libnova/type_traits.hpp>
#include <libnova/value.hpp>

#include <array>
#include <meta>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace nova {

namespace ann {

/**
 * @brief   Member annotation for renaming identifiers.
 *
 * Data must be embedded in the type. "Chasing" pointers does not work well with
 * constant expressions.
 */
struct rename {
    std::array<char, 64> data{};

    consteval rename() = default;

    consteval rename(const char* s) {
        std::size_t i = 0;
        for (; s[i] != '\0' && i + 1 < data.size(); ++i) {
            data[i] = s[i];
        }
    }
};

} // namespace ann

namespace detail {

/**
 * @brief   Dancing with constant-expressions. (Magick)
 */
template <std::meta::info Member>
consteval auto rename() -> std::string_view {
    constexpr auto ann_rename = refl::extract_annotation<ann::rename>(Member);
    constexpr auto storage = std::define_static_array(ann_rename.data);
    return std::string_view(storage.data());
}

/**
 * @brief   Name of the member.
 *
 * @returns with the identifier of the field or the annotated name.
 */
template <std::meta::info Member>
consteval auto member_name() -> std::string_view {
    if constexpr (refl::has_annotation_of<ann::rename>(Member)) {
        return rename<Member>();
    } else {
        return std::meta::identifier_of(Member);
    }
}

template <typename T, typename Traits>
    requires std::is_enum_v<T>
auto bind_impl(const basic_value<Traits>& v) -> T;

template <typename T, typename Traits>
    requires std::same_as<T, std::string>
auto bind_impl(const basic_value<Traits>& v) -> T;

template <typename T, typename Traits>
    requires sequence_like<T>
auto bind_impl(const basic_value<Traits>& v) -> T;

template <typename T, typename Traits>
    requires map_like<T>
auto bind_impl(const basic_value<Traits>& v) -> T;

template <typename T, typename Traits>
    requires (
        std::meta::is_class_type(^^T)
        && not sequence_like<T>
        && not map_like<T>
        && not std::same_as<T, std::string>
    )
auto bind_impl(const basic_value<Traits>& v) -> T;

template <typename T, typename Traits>
auto bind_impl(const basic_value<Traits>& v) -> T;

/**
 * @brief   Binds a single aggregate member of `T` from `map`, by its (possibly renamed) key.
 */
template <std::meta::info Member, typename T, typename Traits>
void bind_member(T& out, const typename basic_value<Traits>::map_t& map) {
    using M = [: std::meta::type_of(Member) :];

    constexpr std::string_view name = member_name<Member>();

    auto it = map.find(name);
    const basic_value<Traits>* mv = it == map.end() ? nullptr : &it->second;

    if (not mv) {
        if constexpr (is_optional_v<M> || std::meta::has_default_member_initializer(Member)) {
            return;
        } else {
            throw exception("Missing required key '{}'", name);
        }
    }

    if constexpr (is_optional_v<M>) {
        out.[: Member :] = bind_impl<typename M::value_type>(*mv);
    } else {
        out.[: Member :] = bind_impl<M>(*mv);
    }
}

template <typename T, typename Traits>
    requires std::is_enum_v<T>
auto bind_impl(const basic_value<Traits>& v) -> T {
    return refl::string_to_enum<T>(v.template as<std::string>());
}

template <typename T, typename Traits>
    requires std::same_as<T, std::string>
auto bind_impl(const basic_value<Traits>& v) -> T {
    return v.template as<std::string>();
}

template <typename T, typename Traits>
    requires sequence_like<T>
auto bind_impl(const basic_value<Traits>& v) -> T {
    T out;
    for (const auto& elem : v.as_seq()) {
        out.push_back(bind_impl<typename T::value_type>(elem));
    }
    return out;
}

template <typename T, typename Traits>
    requires map_like<T>
auto bind_impl(const basic_value<Traits>& v) -> T {
    T out;
    for (const auto& [key, val] : v.as_map()) {
        out.emplace(key, bind_impl<typename T::mapped_type>(val));
    }
    return out;
}

template <typename T, typename Traits>
    requires (
        std::meta::is_class_type(^^T)
        && not sequence_like<T>
        && not map_like<T>
        && not std::same_as<T, std::string>
    )
auto bind_impl(const basic_value<Traits>& v) -> T {
    T out{};
    const auto& map = v.as_map();

    constexpr auto mctx = std::meta::access_context::current();
    template for (constexpr auto mem : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, mctx))) {
        bind_member<mem, T, Traits>(out, map);
    }

    return out;
}

template <typename T, typename Traits>
auto bind_impl(const basic_value<Traits>& v) -> T {
    return v.template as<T>();
}

template <typename Traits, typename T>
    requires std::is_enum_v<T>
auto unbind_impl(const T& t) -> basic_value<Traits>;

template <typename Traits, typename T>
    requires (std::same_as<T, std::string> || std::same_as<T, bool>)
auto unbind_impl(const T& t) -> basic_value<Traits>;

template <typename Traits, typename T>
    requires sequence_like<T>
auto unbind_impl(const T& t) -> basic_value<Traits>;

template <typename Traits, typename T>
    requires map_like<T>
auto unbind_impl(const T& t) -> basic_value<Traits>;

template <typename Traits, typename T>
    requires (
        std::meta::is_class_type(^^T)
        && not sequence_like<T>
        && not map_like<T>
        && not std::same_as<T, std::string>
    )
auto unbind_impl(const T& t) -> basic_value<Traits>;

template <typename Traits, typename T>
    requires (std::integral<T> && not std::same_as<T, bool>)
auto unbind_impl(const T& t) -> basic_value<Traits>;

template <typename Traits, typename T>
    requires std::floating_point<T>
auto unbind_impl(const T& t) -> basic_value<Traits>;

template <typename Traits, typename T>
auto unbind_impl(const T& t) -> basic_value<Traits>;

/**
 * @brief   Unbinds a single aggregate member of `t` into `map`, under its (possibly renamed) key.
 */
template <std::meta::info Member, typename Traits, typename T>
void unbind_member(typename basic_value<Traits>::map_t& map, const T& t) {
    using M = [: std::meta::type_of(Member) :];
    constexpr std::string_view name = member_name<Member>();

    if constexpr (is_optional_v<M>) {
        if (t.[: Member :]) {
            map.emplace(std::string(name), unbind_impl<Traits>(*t.[: Member :]));
        }
    } else {
        map.emplace(std::string(name), unbind_impl<Traits>(t.[: Member :]));
    }
}

template <typename Traits, typename T>
    requires std::is_enum_v<T>
auto unbind_impl(const T& t) -> basic_value<Traits> {
    return basic_value<Traits>(std::string(refl::enum_to_string(t)));
}

template <typename Traits, typename T>
    requires (std::same_as<T, std::string> || std::same_as<T, bool>)
auto unbind_impl(const T& t) -> basic_value<Traits> {
    return basic_value<Traits>(t);
}

template <typename Traits, typename T>
    requires sequence_like<T>
auto unbind_impl(const T& t) -> basic_value<Traits> {
    typename basic_value<Traits>::seq_t seq;
    for (const auto& elem : t) {
        seq.push_back(unbind_impl<Traits>(elem));
    }
    return basic_value<Traits>(std::move(seq));
}

template <typename Traits, typename T>
    requires map_like<T>
auto unbind_impl(const T& t) -> basic_value<Traits> {
    typename basic_value<Traits>::map_t map;
    for (const auto& [key, val] : t) {
        map.emplace(key, unbind_impl<Traits>(val));
    }
    return basic_value<Traits>(std::move(map));
}

template <typename Traits, typename T>
    requires (
        std::meta::is_class_type(^^T)
        && not sequence_like<T>
        && not map_like<T>
        && not std::same_as<T, std::string>
    )
auto unbind_impl(const T& t) -> basic_value<Traits> {
    typename basic_value<Traits>::map_t map;

    constexpr auto mctx = std::meta::access_context::current();
    template for (constexpr auto mem : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, mctx))) {
        unbind_member<mem, Traits>(map, t);
    }

    return basic_value<Traits>(std::move(map));
}

template <typename Traits, typename T>
    requires (std::integral<T> && not std::same_as<T, bool>)
auto unbind_impl(const T& t) -> basic_value<Traits> {
    return basic_value<Traits>(static_cast<std::int64_t>(t));
}

template <typename Traits, typename T>
    requires std::floating_point<T>
auto unbind_impl(const T& t) -> basic_value<Traits> {
    return basic_value<Traits>(static_cast<double>(t));
}

template <typename Traits, typename T>
auto unbind_impl(const T& t) -> basic_value<Traits> {
    return basic_value<Traits>(t);
}

} // namespace detail

/**
 * @brief   Bind an attribute value tree onto a `T`.
 *
 * `T` can be a scalar, an enum (via `refl::string_to_enum`), a sequence
 * type (anything with `push_back`/`value_type`), or an aggregate whose
 * members are bound recursively from a map node. Members use their
 * identifier as the key unless annotated with `[[=attr::ann::rename{"..."}]]`,
 * and may be skipped when absent if they are `std::optional<U>` or have a
 * default member initializer.
 */
template <typename T, typename Traits>
auto bind(const basic_value<Traits>& v) -> T {
    return detail::bind_impl<T>(v);
}

/**
 * @brief   The reverse of `bind`: turn a `T` into an attribute value tree.
 *
 * Mirrors `bind`'s cases: an enum becomes its name (via `refl::enum_to_string`),
 * a sequence becomes a seq node, and an aggregate becomes a map node keyed the
 * same way `bind` reads it back (identifier, or `[[=attr::ann::rename{"..."}]]`
 * if annotated). A `std::optional<U>` member is left out of the map entirely
 * when empty, rather than written out as some "no value" placeholder.
 */
template <typename Traits = default_value_traits, typename T>
auto unbind(const T& t) -> basic_value<Traits> {
    return detail::unbind_impl<Traits>(t);
}

} // namespace nova

#endif // NOVA_EXPERIMENTAL_FEATURE_SET
