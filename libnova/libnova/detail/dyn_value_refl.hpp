/**
 * Part of Nova C++ Library
 *
 * @brief   Reflection-based binding of `dyn_value` trees onto typed aggregates,
 *          enums and containers, and back.
 *
 * Internal until its public API is designed. Requires
 * `NOVA_EXPERIMENTAL_FEATURE_SET` (reflection); the header is empty without it.
 *
 * @author      Gábor Krisztián Girhiny
 * @coauthor    Claude Opus 5.5
 * @created     2026-10-03
 * @updated     2026-10-31
 */

#pragma once

#ifdef NOVA_EXPERIMENTAL_FEATURE_SET

#include <libnova/dyn_value.hpp>
#include <libnova/error.hpp>
#include <libnova/refl.hpp>
#include <libnova/type_traits.hpp>

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <meta>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace nova::detail {

namespace ann {

/**
 * @brief   Member annotation for the key of a member, if not its identifier.
 *
 * The name is stored in the annotation itself: pointers do not work well in
 * constant expressions.
 */
struct rename {
    std::array<char, 64> data{ };

    consteval rename() = default;

    consteval rename(const char* name) {
        for (std::size_t index = 0; name[index] != '\0' and index + 1 < data.size(); ++index) {
            data[index] = name[index];
        }
    }
};

} // namespace ann

namespace binding {

    template <std::meta::info Member>
    consteval auto renamed() -> std::string_view {
        constexpr auto annotation = refl::extract_annotation<ann::rename>(Member);
        constexpr auto storage = std::define_static_array(annotation.data);
        return std::string_view(storage.data());
    }

    /**
     * @brief   The key of `Member`: its identifier, or its `ann::rename`.
     */
    template <std::meta::info Member>
    consteval auto key_of() -> std::string_view {
        if constexpr (refl::has_annotation_of<ann::rename>(Member)) {
            return renamed<Member>();
        } else {
            return std::meta::identifier_of(Member);
        }
    }

    template <typename T>
    concept aggregate_c =
        std::meta::is_class_type(^^T)
        and not sequence_like<T>
        and not map_like<T>
        and not std::same_as<T, std::string>
        and not std::same_as<T, dyn_value>;

    [[noreturn]] inline void throw_mismatch(const dyn_path& path, std::string expected, const dyn_value& node) {
        throw exception("{}", dyn_mismatch{ .where = path, .expected = std::move(expected), .got = std::string(node.type_name()) });
    }

    template <typename T>
    auto bind(const dyn_value& node, const dyn_path& path) -> T;

    template <std::meta::info Member, typename T>
    void bind_member(T& out, const dyn_value::map_t& map, const dyn_path& path) {
        using member_t = [: std::meta::type_of(Member) :];
        constexpr auto key = key_of<Member>();

        const auto found = map.find(key);
        if (found == map.end() or found->second.is_null()) {
            if constexpr (is_optional_v<member_t> or std::meta::has_default_member_initializer(Member)) {
                return;
            } else {
                throw exception("Missing `{}`", path / key);
            }
        }

        if constexpr (is_optional_v<member_t>) {
            out.[: Member :] = binding::bind<typename member_t::value_type>(found->second, path / key);
        } else {
            out.[: Member :] = binding::bind<member_t>(found->second, path / key);
        }
    }

    template <typename T>
    auto bind(const dyn_value& node, const dyn_path& path) -> T {
        if constexpr (std::is_enum_v<T>) {
            const auto* name = std::get_if<std::string>(&node.data());
            if (name == nullptr) {
                throw_mismatch(path, "string", node);
            }
            try {
                return refl::string_to_enum<T>(*name);
            } catch (const exception& ex) {
                throw exception("`{}`: {}", path, ex.what());
            }
        } else if constexpr (sequence_like<T>) {
            const auto* seq = std::get_if<dyn_value::seq_t>(&node.data());
            if (seq == nullptr) {
                throw_mismatch(path, "seq", node);
            }
            auto ret = T{ };
            for (std::size_t index = 0; const auto& item : *seq) {
                ret.push_back(binding::bind<typename T::value_type>(item, path / index++));
            }
            return ret;
        } else if constexpr (map_like<T>) {
            const auto* map = std::get_if<dyn_value::map_t>(&node.data());
            if (map == nullptr) {
                throw_mismatch(path, "map", node);
            }
            auto ret = T{ };
            for (const auto& [key, item] : *map) {
                if (not item.is_null()) {
                    ret.emplace(key, binding::bind<typename T::mapped_type>(item, path / key));
                }
            }
            return ret;
        } else if constexpr (aggregate_c<T>) {
            const auto* map = std::get_if<dyn_value::map_t>(&node.data());
            if (map == nullptr) {
                throw_mismatch(path, "map", node);
            }
            auto ret = T{ };
            constexpr auto context = std::meta::access_context::current();
            template for (constexpr auto member : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, context))) {
                bind_member<member>(ret, *map, path);
            }
            return ret;
        } else {
            auto ret = dyn_convert<T>::convert(node);
            if (not ret) {
                auto mismatch = ret.error();
                mismatch.where = path / mismatch.where;
                throw exception("{}", mismatch);
            }
            return std::move(*ret);
        }
    }

    template <typename T>
    auto unbind(const T& value) -> dyn_value;

    template <std::meta::info Member, typename T>
    void unbind_member(dyn_value::map_t& map, const T& value) {
        using member_t = [: std::meta::type_of(Member) :];
        constexpr auto key = key_of<Member>();

        if constexpr (is_optional_v<member_t>) {
            if (value.[: Member :]) {
                map.emplace(std::string(key), binding::unbind(*value.[: Member :]));
            }
        } else {
            map.emplace(std::string(key), binding::unbind(value.[: Member :]));
        }
    }

    template <typename T>
    auto unbind(const T& value) -> dyn_value {
        if constexpr (std::is_enum_v<T>) {
            return std::string(refl::enum_to_string(value));
        } else if constexpr (sequence_like<T>) {
            auto ret = dyn_value::seq_t{ };
            for (const auto& item : value) {
                ret.push_back(binding::unbind(item));
            }
            return ret;
        } else if constexpr (map_like<T>) {
            auto ret = dyn_value::map_t{ };
            for (const auto& [key, item] : value) {
                ret.emplace(key, binding::unbind(item));
            }
            return ret;
        } else if constexpr (aggregate_c<T>) {
            auto ret = dyn_value::map_t{ };
            constexpr auto context = std::meta::access_context::current();
            template for (constexpr auto member : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, context))) {
                unbind_member<member>(ret, value);
            }
            return ret;
        } else {
            return dyn_value(value);
        }
    }

} // namespace binding

/**
 * @brief   `node` as a `T`.
 *
 * `T` can be anything `dyn_value::lookup` takes, an enum (by the names of its
 * enumerators), a sequence (anything with `push_back` and `value_type`), a map
 * with string keys, or an aggregate whose members bind recursively from a map.
 * A member's key is its identifier, or its `ann::rename` annotation. A member
 * may be missing (or null) if it is a `std::optional` or has a default member
 * initializer.
 *
 * @throws  if a required member is missing or a value has the wrong type; the
 *          message names the full path.
 */
template <typename T>
auto bind(const dyn_value& node) -> T {
    return binding::bind<T>(node, { });
}

/**
 * @brief   `value` as a `dyn_value`; the reverse of `bind`.
 *
 * An empty `std::optional` member is left out.
 */
template <typename T>
auto unbind(const T& value) -> dyn_value {
    return binding::unbind(value);
}

} // namespace nova::detail

#endif // NOVA_EXPERIMENTAL_FEATURE_SET
