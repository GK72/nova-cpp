/**
 * Part of Nova C++ Library.
 *
 * Reimplementatino of `std::expected` because not all compilers
 * support it yet.
 *
 * CAUTION: heavily unstable implementation!
 */

#pragma once

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

    // constexpr expected(unexpected_type&& unexpected)
        // : _value_or_unexpected(std::move(unexpected.value))
    // {}

    template <typename E2>
        requires std::is_convertible_v<typename E2::type, E>
    constexpr expected(const E2& unexpected)
        : m_vex(detail::unexpect, unexpected.value)
    {}

    // template <typename E2>
        // requires std::is_convertible_v<typename E2::type, E>
    // constexpr expected(E2&& unexpected)
        // : _value_or_unexpected(std::move(unexpected.value))
    // {}

    [[nodiscard]] constexpr const T*  operator->() const   noexcept { return std::addressof(m_vex.impl.v); }
    [[nodiscard]] constexpr       T*  operator->()         noexcept { return std::addressof(m_vex.impl.v); }
    [[nodiscard]] constexpr const T&  operator*()  const&  noexcept { return m_vex.impl.v; }
    [[nodiscard]] constexpr       T&  operator*()       &  noexcept { return m_vex.impl.v; }
    // [[nodiscard]] constexpr const T&& operator*()  const&& noexcept { return std::move(m_vex.impl.v); }
    // [[nodiscard]] constexpr       T&& operator*()       && noexcept { return std::move(m_vex.impl.v); }

    [[nodiscard]] constexpr bool has_value() const noexcept         { return m_vex.value; }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return has_value(); }

    [[nodiscard]] constexpr const T& value()  const&                { return m_vex.impl.v; }
    [[nodiscard]] constexpr       T& value()       &                { return m_vex.impl.v; }
    // [[nodiscard]] constexpr const T&& value() const&&               { return std::move(m_vex.impl.v); }
    // [[nodiscard]] constexpr       T&& value()      &&               { return std::move(m_vex.impl.v); }

    [[nodiscard]] constexpr const E& error() const&                 { return m_vex.impl.e; }
    // [[nodiscard]] constexpr E& error() &                            { return std::get<E>(_value_or_unexpected); }
    // [[nodiscard]] constexpr const E&& error() const&&               { return std::get<E>(_value_or_unexpected); }
    // [[nodiscard]] constexpr E&& error() &&                          { return std::get<E>(_value_or_unexpected); }

    template <typename U>
    [[nodiscard]] constexpr
    T value_or(U&& def) const& {
        // TODO: copy constructible
        // TODO: convertible U to T
        return has_value() ? value() : static_cast<T>(std::forward<U>(def));
    }

    template <typename U>
    [[nodiscard]] constexpr
    T value_or(U&& def) && {
        // TODO: move constructible
        // TODO: convertible U to T
        return has_value() ? std::move(value()) : static_cast<T>(std::forward<U>(def));
    }

    template <typename U = E>
    [[nodiscard]] constexpr
    E error_or(U&& def) const& {
        // TODO: copy constructible
        // TODO: convertible U to E
        return has_value() ? std::forward<U>(def) : error();
    }

    template <typename U = E>
    [[nodiscard]] constexpr
    E error_or(U&& def) && {
        // TODO: move constructible
        // TODO: convertible U to E
        return has_value() ? std::forward<U>(def) : std::move(error());
    }

    // TODO(feat): and_then
    // TODO(feat): transform
    // TODO(feat): or_else
    // TODO(feat): transform_error

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

} // namespace nova
