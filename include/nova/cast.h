/**
 * Part of Nova C++ Library.
 *
 * Safe and unsafe casting.
 */

#pragma once

#include "nova/parse.h"
#include "nova/type_traits.h"

#include <stdexcept>
#include <type_traits>

namespace nova {

namespace detail {

    template <typename Func>
    auto throw_if_parse_error(Func parse) -> decltype(auto) {
        const auto ret = parse();
        if (not ret.has_value()) {
            throw std::runtime_error("Cast failed!");
        }
        return *ret;
    }

    template <typename R, typename T>
    [[nodiscard]] auto as(T&& x) -> decltype(auto) {
        if constexpr (std::is_convertible_v<T, R>) {
            return static_cast<R>(x);
        } else if constexpr (std::is_constructible_v<R, T>) {
            return static_cast<R>(x);
        }
        else {
            static_assert(dependent_false<T>, "Cast is not allowed!");
        }
    }

    template <typename R, typename T>
        requires std::is_arithmetic_v<R> && string_like<T>
    [[nodiscard]] auto as(T&& x) -> decltype(auto) {
        return throw_if_parse_error([y = std::forward<T>(x)]() { return to_number<R>(y); });
    }

    template <typename R, typename T>
        requires chrono_duration<R> && string_like<T>
    [[nodiscard]] auto as(T&& x) -> decltype(auto) {
        return throw_if_parse_error([y = std::forward<T>(x)]() { return to_chrono<R>(y); });
    }

    /**
     * @brief   Helper function for implicit type casting for rvalue references.
     *
     * ```
     * int x = 1;
     * short y = nova::as(x);
     * ```
     */
    // template <typename T>
    // struct as_fn {
        // as_fn(T&& x_) : x(std::forward<T>(x_)) {}

        // template <typename R>
        // operator R() {
            // return as<R>(std::forward<T>(x));
        // }

        // T&& x;
    // };

    /**
     * @brief   Helper function for implicit type casting for lvalue references.
     *
     * ```
     * int x = 1;
     * short y = nova::as(x);
     * ```
     */
    template <typename T>
    struct as_fn_ref {
        as_fn_ref(T& x_) : x(x_) {}

        template <typename R>
        operator R() {
            return as<R>(x);
        }

        T& x;
    };

} // namespace detail

namespace alpha {

    /**
     * @brief   Uniform casting with explicit type (exposition only)
     *
     * NOTE: alpha version!
     */
    template <typename R, typename T>
    [[nodiscard]] auto as(T&& x) -> decltype(auto) {
        return detail::as<R>(std::forward<T>(x));
    }

    /**
     * @brief   Uniform casting with implicit type (exposition only)
     *
     * NOTE: alpha version!
     *
     * TODO(design): error handling; should `as` fail, and if yes, how?
     */
    template <typename T>
    [[nodiscard]] auto as(T&& x) -> decltype(auto) {
        // TODO: find a use-case
        // if constexpr (std::is_rvalue_reference_v<decltype(x)>) {
            // return detail::as_fn(std::move(x));
        // } else {
            return detail::as_fn_ref(x);
        // }
    }

} // namespace alpha

} // namespace nova
