/**
 * Part of Nova C++ Library.
 *
 * Reimplementatino of `std::expected` because not all compilers
 * support it yet.
 *
 * CAUTION: experimental and incomplete implementation!
 */

#pragma once

#include <functional>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace nova {

namespace detail {

    // Tags for choosing the member in the `union`.

    struct expect_t   { explicit expect_t()   = default; };
    struct unexpect_t { explicit unexpect_t() = default; };

    inline constexpr expect_t expect;
    inline constexpr unexpect_t unexpect;

} // namespace detail

using detail::unexpect;

template <typename T, typename E>
class expected;

template <typename T>
struct is_expected : std::false_type {};

template <typename T, typename E>
struct is_expected<expected<T, E>> : std::true_type {};

template <typename T>
inline constexpr bool is_expected_v = is_expected<T>::value;

struct empty {};

template <typename E>
struct unexpected {
    using type = E;
    E value;
};

template <typename T, typename E>
class expected {
public:
    using value_type      = T;
    using error_type      = E;
    using unexpected_type = unexpected<E>;

    constexpr expected(const value_type& value)
        : m_vex(detail::expect, value)
    {}

    constexpr expected(value_type&& value)
        : m_vex(detail::expect, std::move(value))
    {}

    constexpr expected(const unexpected_type& unexpected)
        : m_vex(detail::unexpect, unexpected.value)
    {}

    template <typename E2>
        requires std::is_convertible_v<typename E2::type, E>
    constexpr expected(const E2& unexpected)
        : m_vex(detail::unexpect, unexpected.value)
    {}

    /**
     * @brief   Construct a value in place.
     */
    template <typename ...Args>
    constexpr expected(std::in_place_t, Args&&... args)                                             // NOLINT(readability-named-parameter) | It's a tag
        : m_vex(detail::expect, std::forward<Args>(args)...)
    {}

    /**
     * @brief   Construct an unexpected value in place.
     */
    template <typename ...Args>
    constexpr expected(detail::unexpect_t, Args&&... args)                                          // NOLINT(readability-named-parameter) | It's a tag
        : m_vex(detail::unexpect, std::forward<Args>(args)...)
    {}

    constexpr expected(const expected& other)
        requires std::is_copy_constructible_v<T>
             and std::is_copy_constructible_v<E>
             and std::is_trivially_copy_constructible_v<T>
             and std::is_trivially_copy_constructible_v<E>
        = default;

    constexpr expected(const expected& other)
        requires std::is_copy_constructible_v<T>
             and std::is_copy_constructible_v<E>
             and (not (std::is_trivially_copy_constructible_v<T>
                 and std::is_trivially_copy_constructible_v<E>
             ))
        : m_vex(other.has_value(), other.m_vex)
    {}

    constexpr expected(expected&& other)
        requires std::is_move_constructible_v<T>
             and std::is_move_constructible_v<E>
             and std::is_trivially_move_constructible_v<T>
             and std::is_trivially_move_constructible_v<E>
        = default;

    constexpr expected(expected&& other) noexcept
        requires std::is_move_constructible_v<T>
             and std::is_move_constructible_v<E>
             and (not (std::is_trivially_move_constructible_v<T>
                 and std::is_trivially_move_constructible_v<E>
            ))
        : m_vex(other.has_value(), std::move(other.m_vex))
    {}

    [[nodiscard]] constexpr const T*  operator->() const   noexcept { return std::addressof(m_vex.impl.v); }
    [[nodiscard]] constexpr       T*  operator->()         noexcept { return std::addressof(m_vex.impl.v); }
    [[nodiscard]] constexpr const T&  operator*()  const&  noexcept { return m_vex.impl.v; }
    [[nodiscard]] constexpr       T&  operator*()       &  noexcept { return m_vex.impl.v; }
    [[nodiscard]] constexpr const T&& operator*()  const&& noexcept { return std::move(m_vex.impl.v); }
    [[nodiscard]] constexpr       T&& operator*()       && noexcept { return std::move(m_vex.impl.v); }

    [[nodiscard]] constexpr bool has_value() const noexcept         { return m_vex.value; }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return has_value(); }

    [[nodiscard]] constexpr const T& value() const& {
        if (not has_value()) {
            // TODO(feat): throw the error type (feature depends on custom exception type)
            throw std::runtime_error("Bad expected access");
        }
        return m_vex.impl.v;
    }

    [[nodiscard]] constexpr T& value() & {
        if (not has_value()) {
            // TODO(feat): throw the error type (feature depends on custom exception type)
            throw std::runtime_error("Bad expected access");
        }
        return m_vex.impl.v;
    }

    template <typename T2, typename E2>
    [[nodiscard]] friend constexpr bool operator==(const expected& lhs, const expected<T2, E2>& rhs) {
        return (
            lhs.has_value() and rhs.has_value()
                and *lhs == * rhs
            ) or (
            not lhs.has_value() and not rhs.has_value()
                and lhs.error() == rhs.error()
        );
    }

    [[nodiscard]] constexpr const T&& value() const&&               { return std::move(m_vex.impl.v); }
    [[nodiscard]] constexpr       T&& value()      &&               { return std::move(m_vex.impl.v); }

    [[nodiscard]] constexpr const E&  error() const&                { return m_vex.impl.e; }
    [[nodiscard]] constexpr E&        error()      &                { return m_vex.impl.e; }
    [[nodiscard]] constexpr const E&& error() const&&               { return std::move(m_vex.impl.e); }
    [[nodiscard]] constexpr E&&       error()      &&               { return std::move(m_vex.impl.e); }

    template <typename U>
        requires std::is_copy_constructible_v<T>
             and std::is_convertible_v<U, T>
    [[nodiscard]] constexpr
    T value_or(U&& def) const& {
        return has_value() ? value() : static_cast<T>(std::forward<U>(def));
    }

    template <typename U>
        requires std::is_move_constructible_v<T>
             and std::is_convertible_v<U, T>
    [[nodiscard]] constexpr
    T value_or(U&& def) && {
        return has_value() ? std::move(value()) : static_cast<T>(std::forward<U>(def));
    }

    template <typename U = E>
        requires std::is_copy_constructible_v<U>
             and std::is_convertible_v<U, E>
    [[nodiscard]] constexpr
    E error_or(U&& def) const& {
        return has_value() ? std::forward<U>(def) : error();
    }

    template <typename U = E>
        requires std::is_move_constructible_v<U>
             and std::is_convertible_v<U, E>
    [[nodiscard]] constexpr
    E error_or(U&& def) && {
        return has_value() ? std::forward<U>(def) : std::move(error());
    }

    template <typename Func>
        requires std::is_constructible_v<E, E&>
    [[nodiscard]] constexpr
    auto and_then(Func&& func) & {
        using R = std::remove_cvref_t<std::invoke_result_t<Func, T&>>;
        static_assert(is_expected_v<R>);
        static_assert(std::is_same_v<typename R::error_type, E>);

        if (has_value()) {
            return std::invoke(std::forward<Func>(func), this->operator*());
        }
        return R{ unexpect, error()};
    }

    template <typename Func>
        requires std::is_constructible_v<E, const E&>
    [[nodiscard]] constexpr
    auto and_then(Func&& func) const& {
        using R = std::remove_cvref_t<std::invoke_result_t<Func, const T&>>;
        static_assert(is_expected_v<R>);
        static_assert(std::is_same_v<typename R::error_type, E>);

        if (has_value()) {
            return std::invoke(std::forward<Func>(func), this->operator*());
        }
        return R{ unexpect, error()};
    }

    template <typename Func>
        requires std::is_constructible_v<E, const E&&>
    [[nodiscard]] constexpr
    auto and_then(Func&& func) const&& {
        using R = std::remove_cvref_t<std::invoke_result_t<Func, const T&&>>;
        static_assert(is_expected_v<R>);
        static_assert(std::is_same_v<typename R::error_type, E>);

        if (has_value()) {
            return std::invoke(std::forward<Func>(func), std::move(this->operator*()));
        }
        return R{ unexpect, std::move(error())};
    }

    template <typename Func>
        requires std::is_constructible_v<E, E&&>
    [[nodiscard]] constexpr
    auto and_then(Func&& func) && {
        using R = std::remove_cvref_t<std::invoke_result_t<Func, T&&>>;
        static_assert(is_expected_v<R>);
        static_assert(std::is_same_v<typename R::error_type, E>);

        if (has_value()) {
            return std::invoke(std::forward<Func>(func), std::move(this->operator*()));
        }
        return R{ unexpect, std::move(error())};
    }

    template <typename Func>
        requires std::is_constructible_v<T, T&>
    [[nodiscard]] constexpr
    auto or_else(Func&& func) & {
        using R = std::remove_cvref_t<std::invoke_result_t<Func, E&>>;
        static_assert(is_expected_v<R>);
        static_assert(std::is_same_v<typename R::value_type, T>);

        if (has_value()) {
            return R{ this->operator*() };
        }
        return std::invoke(std::forward<Func>(func), error());
    }

    template <typename Func>
        requires std::is_constructible_v<T, const T&>
    [[nodiscard]] constexpr
    auto or_else(Func&& func) const& {
        using R = std::remove_cvref_t<std::invoke_result_t<Func, const E&>>;
        static_assert(is_expected_v<R>);
        static_assert(std::is_same_v<typename R::value_type, T>);

        if (has_value()) {
            return R{ this->operator*() };
        }
        return std::invoke(std::forward<Func>(func), error());
    }

    template <typename Func>
        requires std::is_constructible_v<T, T&&>
    [[nodiscard]] constexpr
    auto or_else(Func&& func) && {
        using R = std::remove_cvref_t<std::invoke_result_t<Func, E&&>>;
        static_assert(is_expected_v<R>);
        static_assert(std::is_same_v<typename R::value_type, T>);

        if (has_value()) {
            return R{ std::move(this->operator*()) };
        }
        return std::invoke(std::forward<Func>(func), std::move(error()));
    }

    template <typename Func>
        requires std::is_constructible_v<T, const T&&>
    [[nodiscard]] constexpr
    auto or_else(Func&& func) const&& {
        using R = std::remove_cvref_t<std::invoke_result_t<Func, const E&&>>;
        static_assert(is_expected_v<R>);
        static_assert(std::is_same_v<typename R::value_type, T>);

        if (has_value()) {
            return R{ std::move(this->operator*()) };
        }
        return std::invoke(std::forward<Func>(func), std::move(error()));
    }

private:
    union vex_impl {
        template <typename ...Args>
        constexpr vex_impl(detail::expect_t, Args&& ...args)                                        // NOLINT(readability-named-parameter) | It's a tag
            : v(std::forward<Args>(args)...)
        {}

        template <typename ...Args>
        constexpr vex_impl(detail::unexpect_t, Args&& ...args)                                      // NOLINT(readability-named-parameter) | It's a tag
            : e(std::forward<Args>(args)...)
        {}

        constexpr ~vex_impl() requires std::is_trivially_destructible_v<T>
                                    and std::is_trivially_destructible_v<E>
                              = default;

        /**
         * @brief   Outer class, `vex` knows how to destruct.
         *
         * Information about which member is active is carried outside this union.
         */
        constexpr ~vex_impl() { /* `vex` destructs */ }

        T v;
        E e;
    };

    /**
    * @brief    Value or unEXpected.
    *
    * Contains the `union` in a (somewhat) safe manner.
    */
    struct vex {
        template <typename Other>
        constexpr vex(bool has_value, Other&& other)
            : impl(make_union(has_value, std::forward<Other>(other)))
            , value(has_value)
        {}

        template <typename ...Args>
        constexpr vex(detail::expect_t tag, Args&& ...args)
            : impl(tag, std::forward<Args>(args)...)
            , value(true)
        {}

        template <typename ...Args>
        constexpr vex(detail::unexpect_t tag, Args&& ...args)
            : impl(tag, std::forward<Args>(args)...)
            , value(false)
        {}

        template <typename Other>
        constexpr vex_impl make_union(bool has_value, Other&& other) {
            if (has_value) {
                return vex_impl{ detail::expect, std::forward<Other>(other).impl.v };
            }
            return vex_impl{ detail::unexpect, std::forward<Other>(other).impl.e };
        }

        constexpr ~vex() requires std::is_trivially_destructible_v<T>
                              and std::is_trivially_destructible_v<E>
                         = default;

        constexpr ~vex() {
            if (value) {
                std::destroy_at(std::addressof(impl.v));
            } else {
                std::destroy_at(std::addressof(impl.e));
            }
        }

        vex_impl impl;
        bool value;
    } m_vex;

};

} // namespace nova
