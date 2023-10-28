#pragma once

#include <array>
#include <compare>
#include <cstddef>
#include <string>
#include <tuple>
#include <utility>

namespace nova {
    template <std::size_t Size> class static_string;
}

namespace std {
    template <size_t ...N>
    constexpr auto tuple_cat(const nova::static_string<N>& ...strs) noexcept;
}

namespace nova {

/**
 * @brief   Compile time string
 *
 * For NTTP (non-type template parameter) usage
 * this is a *structural type* (all data members are public)
 */
template <std::size_t Size>
class static_string {
public:
    using value_type = char;

    /**
     * @brief   Construct from c-string without the null terminator
     */
    template <std::size_t N>
    constexpr static_string(const value_type (&str)[N]) noexcept
        : static_string(str, std::make_index_sequence<N - 1>())
    {}

    /**
     * @brief   Construct from arrays during concatenation
     */
    constexpr static_string(const std::array<value_type, Size> string) noexcept
        : data(string)
    {}

    [[nodiscard]] constexpr auto size() const noexcept                      { return data.size(); }
    [[nodiscard]] constexpr auto operator[](std::size_t idx) const noexcept { return data[idx]; }

    template <std::size_t Index>
    [[nodiscard]] constexpr auto get() const noexcept { return data[Index]; }

    operator std::string() {
        return std::string{ data.data(), data.size() };
    }

    std::array<value_type, Size> data;                                                              // NOLINT(misc-non-private-member-variables-in-classes): NTTP

private:
    /**
     * @brief   Copy the content of a c-string into a `std::array`
     *
     * Helper constructor
     */
    template <std::size_t N, std::size_t ...Index>
    constexpr static_string(const value_type (&str)[N], std::index_sequence<Index...>) noexcept
        : data({ str[Index]... })
    {}

};

// TODO(traits): all combinations, possibly with `std::decay`
template <typename T>    struct is_static_string : std::false_type {};
template <std::size_t N> struct is_static_string<static_string<N>> : std::true_type {};
template <std::size_t N> struct is_static_string<static_string<N>&> : std::true_type {};
template <std::size_t N> struct is_static_string<char[N]> : std::true_type {};
template <std::size_t N> struct is_static_string<const char[N]> : is_static_string<char[N]> {};
template <std::size_t N> struct is_static_string<const char(&)[N]> : is_static_string<char[N]> {};

template <typename ...Ts> struct is_all_static_string : std::conjunction<is_static_string<Ts>...> {};

template <std::size_t N>
static_string(const char (&str)[N]) -> static_string<N - 1>;

/**
 * @brief   Alphabetic comparison
 */
template <std::size_t N, std::size_t M>
[[nodiscard]] constexpr
auto operator<=>(const static_string<N> lhs, const static_string<M> rhs) noexcept {
    std::size_t i = 0;
    while (i < N && i < M) {
        if (lhs[i] < rhs[i]) { return std::strong_ordering::less; }
        if (lhs[i] > rhs[i]) { return std::strong_ordering::greater; }
        ++i;
    }

    if (i < N) { return std::strong_ordering::less; }
    if (i < M) { return std::strong_ordering::greater; }

    return std::strong_ordering::equal;
}

/**
 * @brief   Comparing with spaceship operator
 *
 * Note: only `default`ed spaceship operator syntesizes equality operator
 */
template <std::size_t N, std::size_t M>
[[nodiscard]] constexpr
auto operator==(const static_string<N> lhs, const static_string<M> rhs) noexcept {
    return lhs <=> rhs == std::strong_ordering::equal;
}

template <typename T>
struct separator {
    T value;
};

/**
 * Forbidding using c-strings in `separator` by automagically
 * wrapping it in `static_string`
 */
template <std::size_t N>
separator(const char (&str)[N]) -> separator<decltype(static_string(str))>;

namespace detail {

    /**
     * @brief   Concatenate a string of various length of strings
     */
    template <std::size_t ...N>
    [[nodiscard]] constexpr auto concat(const static_string<N>& ...strings)
            -> static_string<(N + ...)>
    {
        return std::apply(
            [](auto... chars) -> static_string<(N + ...)> {
                return std::array{ chars... };
            },
            std::tuple_cat(
                std::tuple_cat(strings.data)...
            )
        );
    }

    /**
     * @brief   Concatenate compile time if all arguments are static strings
     *
     * @return
     *      `static_string` if it can be done compile time
     *      `std::string`   if it can be only done at runtime
     */
    template <typename ...Ts>
    constexpr auto concat_helper(Ts&&... strings) noexcept {
        if constexpr (is_all_static_string<Ts...>{}) {
            return std::tuple_cat(static_string(strings)...);
        }
        else {
            std::string str;
            str.reserve((std::size(strings) + ...));
            (str.append(std::string(strings)), ...);
            return str;
        }
    }

}

/**
 * @brief   Concatenate either static or dynamic strings
 *
 * TODO(constraints): possible conflict with array `concat` (utils.h`
 */
template <typename Head, typename ...Tail>
[[nodiscard]] constexpr auto concat(Head&& head, Tail&& ...tail) {
    return detail::concat_helper(head, tail...);
}

/**
 * @brief   Concatenate strings with a separator
 */
template <typename T, typename Head, typename ...Tail>
[[nodiscard]] constexpr auto concat(separator<T> separator, Head&& head, Tail&& ...tail) {
    return std::apply(
        [](auto&& ...strings) { return concat(strings...); },
        std::tuple_cat(
            std::tie(head),
            std::tie(separator.value, tail)...
        )
    );
}

namespace literals {
    template <static_string Str>
    constexpr auto operator""_str() {
        return Str;
    }
}

} // namespace nova

namespace std {
    template <size_t ...N>
    constexpr auto tuple_cat(const nova::static_string<N>& ...strs) noexcept {
        return nova::detail::concat(strs...);
    }
} // namespace std
