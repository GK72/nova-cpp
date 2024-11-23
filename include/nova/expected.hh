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

namespace exp {

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

    // TODO(feat): rvalue pair
    // constexpr expected(unexpected_type&& unexpected)
        // : _value_or_unexpected(std::move(unexpected.value))
    // {}

    template <typename E2>
        requires std::is_convertible_v<typename E2::type, E>
    constexpr expected(const E2& unexpected)
        : m_vex(detail::unexpect, unexpected.value)
    {}

    // TODO(feat): rvalue pair
    // template <typename E2>
        // requires std::is_convertible_v<typename E2::type, E>
    // constexpr expected(E2&& unexpected)
        // : _value_or_unexpected(std::move(unexpected.value))
    // {}

    [[nodiscard]] constexpr const T*  operator->() const   noexcept { return std::addressof(m_vex.impl.v); }
    [[nodiscard]] constexpr       T*  operator->()         noexcept { return std::addressof(m_vex.impl.v); }
    [[nodiscard]] constexpr const T&  operator*()  const&  noexcept { return m_vex.impl.v; }
    [[nodiscard]] constexpr       T&  operator*()       &  noexcept { return m_vex.impl.v; }
    // TODO(feat): rvalue pairs
    // [[nodiscard]] constexpr const T&& operator*()  const&& noexcept { return std::move(m_vex.impl.v); }
    // [[nodiscard]] constexpr       T&& operator*()       && noexcept { return std::move(m_vex.impl.v); }

    [[nodiscard]] constexpr bool has_value() const noexcept         { return m_vex.value; }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return has_value(); }

    [[nodiscard]] constexpr const T& value()  const&                { return m_vex.impl.v; }
    [[nodiscard]] constexpr       T& value()       &                { return m_vex.impl.v; }
    // TODO(feat): rvalue pairs
    // [[nodiscard]] constexpr const T&& value() const&&               { return std::move(m_vex.impl.v); }
    // [[nodiscard]] constexpr       T&& value()      &&               { return std::move(m_vex.impl.v); }

    [[nodiscard]] constexpr const E&  error() const&                { return m_vex.impl.e; }
    [[nodiscard]] constexpr E&        error()      &                { return m_vex.impl.e; }
    // TODO(feat): rvalue pairs
    // [[nodiscard]] constexpr const E&& error() const&&               { return std::move(m_vex.impl.e); }
    // [[nodiscard]] constexpr E&&       error()      &&               { return std::move(m_vex.impl.e); }

    template <typename U>
        requires std::is_copy_constructible_v<T>
              && std::is_convertible_v<U, T>
    [[nodiscard]] constexpr
    T value_or(U&& def) const& {
        return has_value() ? value() : static_cast<T>(std::forward<U>(def));
    }

    template <typename U>
        requires std::is_move_constructible_v<T>
              && std::is_convertible_v<U, T>
    [[nodiscard]] constexpr
    T value_or(U&& def) && {
        return has_value() ? std::move(value()) : static_cast<T>(std::forward<U>(def));
    }

    template <typename U = E>
        requires std::is_copy_constructible_v<U>
              && std::is_convertible_v<U, E>
    [[nodiscard]] constexpr
    E error_or(U&& def) const& {
        return has_value() ? std::forward<U>(def) : error();
    }

    template <typename U = E>
        requires std::is_move_constructible_v<U>
              && std::is_convertible_v<U, E>
    [[nodiscard]] constexpr
    E error_or(U&& def) && {
        return has_value() ? std::forward<U>(def) : std::move(error());
    }

    template <typename Func,
              typename R = std::remove_cvref_t<std::invoke_result_t<Func, T&>>
             >
        requires std::is_invocable_r_v<R, Func, T&>
              && is_expected_v<R>
    [[nodiscard]] constexpr
    auto and_then(Func func) const& {
        if (has_value()) {
            return std::invoke(func, this->operator*());
        }
        return R{ unexpected{ error() }};
    }

    template <typename Func,
              typename R = std::remove_cvref_t<std::invoke_result_t<Func, T&&>>
             >
        requires std::is_invocable_r_v<R, Func, T&&>
              && is_expected_v<R>
    [[nodiscard]] constexpr
    auto and_then(Func func) && {
        if (has_value()) {
            return std::invoke(func, std::move(this->operator*()));
        }
        return R{ unexpected{ std::move(error()) }};
    }

    template <typename Func,
              typename R = std::remove_cvref_t<std::invoke_result_t<Func, E&>>
             >
        requires std::is_invocable_r_v<R, Func, E&>
              && is_expected_v<R>
    [[nodiscard]] constexpr
    auto or_else(Func func) const& {
        if (has_value()) {
            return R{ this->operator*() };
        }
        return std::invoke(func, error());
    }

    template <typename Func,
              typename R = std::remove_cvref_t<std::invoke_result_t<Func, E&>>
             >
        requires std::is_invocable_r_v<R, Func, E&>
              && is_expected_v<R>
    [[nodiscard]] constexpr
    auto or_else(Func func) && {
        if (has_value()) {
            return R{ std::move(this->operator*()) };
        }
        return std::invoke(func, std::move(error()));
    }

private:
    union vex_impl {
        template <typename ...Args>
        constexpr vex_impl(detail::expect_t, Args&& ...args)
            : v(std::forward<Args>(args)...)
        {}

        template <typename ...Args>
        constexpr vex_impl(detail::unexpect_t, Args&& ...args)
            : e(std::forward<Args>(args)...)
        {}

        // TODO(feat): Copy and move constructors
        //               default if trivially copyable/movable
        //               else delete

        constexpr ~vex_impl() requires std::is_trivially_destructible_v<T>
                                    && std::is_trivially_destructible_v<E>
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

        // TODO(feat): Copy and move constructors
        //               default if trivially copyable/movable
        //               else delete

        constexpr ~vex() requires std::is_trivially_destructible_v<T>
                               && std::is_trivially_destructible_v<E>
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

} // namespace exp
} // namespace nova
