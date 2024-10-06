/**
 * Part of Nova C++ Library.
 *
 * Reimplementatino of `std::expected` because not all compilers
 * support it yet.
 *
 * CAUTION: heavily unstable implementation!
 */

#pragma once

#include <type_traits>
#include <utility>
#include <variant>

namespace nova {

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

    template <typename U> constexpr expected(const U& value)
        : _value_or_unexpected(value)
    {}

    constexpr expected(const unexpected_type& unexpected)
        : _value_or_unexpected(unexpected.value)
    {}

    constexpr expected(unexpected_type&& unexpected)
        : _value_or_unexpected(std::move(unexpected.value))
    {}

    template <typename E2>
        requires std::is_convertible_v<typename E2::type, E>
    constexpr expected(const E2& unexpected)
        : _value_or_unexpected(unexpected.value)
    {}

    template <typename E2>
        requires std::is_convertible_v<typename E2::type, E>
    constexpr expected(E2&& unexpected)
        : _value_or_unexpected(std::move(unexpected.value))
    {}

    [[nodiscard]] constexpr const T* operator->() const noexcept    { return &value(); }
    [[nodiscard]] constexpr T* operator->() noexcept                { return &value(); }
    [[nodiscard]] constexpr const T& operator*() const& noexcept    { return value(); }
    [[nodiscard]] constexpr T& operator*() & noexcept               { return value(); }
    [[nodiscard]] constexpr const T&& operator*() const&& noexcept  { return value(); }
    [[nodiscard]] constexpr T&& operator*() && noexcept             { return value(); }

    [[nodiscard]] constexpr bool has_value() const noexcept         { return std::holds_alternative<T>(_value_or_unexpected); }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return has_value(); }

    [[nodiscard]] constexpr const T& value() const&                 { return std::get<T>(_value_or_unexpected); }
    [[nodiscard]] constexpr T& value() &                            { return std::get<T>(_value_or_unexpected); }
    // FIXME: expected
    // [[nodiscard]] constexpr const T&& value() const&&               { return std::get<T>(_value_or_unexpected); }
    // [[nodiscard]] constexpr T&& value() &&                          { return std::get<T>(_value_or_unexpected); }

    [[nodiscard]] constexpr const E& error() const&                 { return std::get<E>(_value_or_unexpected); }
    [[nodiscard]] constexpr E& error() &                            { return std::get<E>(_value_or_unexpected); }
    // FIXME: expected
    // [[nodiscard]] constexpr const E&& error() const&&               { return std::get<E>(_value_or_unexpected); }
    // [[nodiscard]] constexpr E&& error() &&                          { return std::get<E>(_value_or_unexpected); }

    template <typename U>
    [[nodiscard]] constexpr
    T value_or(U&& def) const& {
        return has_value() ? value() : static_cast<T>(std::forward<U>(def));
    }

    template <typename U>
    [[nodiscard]] constexpr
    T value_or(U&& def) && {
        return has_value() ? value() : static_cast<T>(std::forward<U>(def));
    }

private:
    std::variant<T, E> _value_or_unexpected;

};

} // namespace nova
