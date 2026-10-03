/**
 * Part of Nova C++ Library
 *
 * Reflection utilities.
 *
 * Requires `NOVA_EXPERIMENTAL_FEATURE_SET` (reflection); this entire header
 * is a no-op without it.
 *
 * @author  Gábor Krisztián Girhiny
 * @date    2026-05-14
 */

#pragma once

#ifdef NOVA_EXPERIMENTAL_FEATURE_SET

#include <libnova/error.hpp>
#include <fmt/format.h>

#include <meta>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace nova {
namespace ann {

struct fmt_debug_t {};
inline constexpr fmt_debug_t fmt_debug{};

} // namespace ann

namespace refl {

/**
 * @brief   Check whether the target has an annotation of type `AnnType`.
 */
template <typename AnnType>
consteval auto has_annotation_of(std::meta::info target) -> bool {
    for (auto _ : std::meta::annotations_of_with_type(target, ^^AnnType)) {
        return true;
    }
    return false;
}

/**
 * @brief   Pull the first annotation of type `AnnType` off the target, by value.
 *
 * Throws if none is present; check with `has_annotation_of` first if that's expected.
 */
template <typename AnnType>
consteval auto extract_annotation(std::meta::info target) -> AnnType {
    for (auto a : std::meta::annotations_of_with_type(target, ^^AnnType)) {
        return std::meta::extract<AnnType>(a);
    }
    throw nova::exception("No annotation of the requested type found");
}

struct formatter {
    constexpr auto parse(auto& ctx) {
        return ctx.begin();
    }

    template <typename T>
        requires (has_annotation_of<ann::fmt_debug_t>(^^T))
    auto format(const T& type, auto& ctx) const {
        constexpr auto identifier = std::meta::has_identifier(^^T)
            ? std::meta::identifier_of(^^T)
            : "(unnamed-type)";

        auto out = fmt::format_to(ctx.out(), "{}{{ ", identifier);

        auto delim = [first = true, &out]() mutable {
            if (not first) {
                *out++ = ',';
                *out++ = ' ';
            }

            first = false;
        };

        constexpr auto mctx = std::meta::access_context::current();

        template for (constexpr auto base : std::define_static_array(std::meta::bases_of(^^T, mctx))) {
            delim();

            using U = [: std::meta::type_of(base) :];
            out = fmt::format_to(out, "{}", static_cast<const U&>(type));
        }

        template for (constexpr auto mem : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, mctx))) {
            delim();
            std::string_view mem_label = std::meta::has_identifier(mem)
                ? std::meta::identifier_of(mem)
                : "(unnamed-member)";

            out = fmt::format_to(out, ".{}={}", mem_label, type.[: mem :]);
        }

        *out++ = ' ';
        *out++ = '}';

        return out;
    }

};

template <typename E>
    requires (std::meta::is_enumerable_type(^^E))
constexpr auto enum_to_string(E value) -> std::string_view {
    template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^E))) {
        if (value == [: e :]) {
            return std::meta::identifier_of(e);
        }
    }

    return "(unnamed)";
}

template <typename E>
    requires (std::meta::is_enumerable_type(^^E))
constexpr auto string_to_enum(std::string_view value) {
    template for (constexpr auto e : std::define_static_array(std::meta::enumerators_of(^^E))) {
        if (value == std::meta::identifier_of(e)) {
            return [: e :];
        }
    }

    throw nova::exception("Invalid enumeration: {}", value);
}

} // namespace refl
} // namespace nova

namespace fmt {

template <typename T>
    requires (nova::refl::has_annotation_of<nova::ann::fmt_debug_t>(^^T))
struct formatter<T> : nova::refl::formatter {};

} // namespace fmt

#endif // NOVA_EXPERIMENTAL_FEATURE_SET
