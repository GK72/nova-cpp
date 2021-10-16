#pragma once

#include <string>
#include <string_view>
#include <iostream>         // TODO(C++20): use format lib
#include <source_location>
#include <type_traits>

namespace utf {
    constexpr auto checkMark = "\u2713";
    constexpr auto ballot    = "\u2717";

    namespace heavy {
        constexpr auto checkMark = "\u2714";
        constexpr auto ballot    = "\u2718";
    }
}

namespace term_colors {
    inline namespace color256 {
        constexpr auto def = "\033[0m";

        inline namespace fg {
            constexpr auto black    = "\033[38;5;0m";
            constexpr auto red      = "\033[38;5;1m";
            constexpr auto green    = "\033[38;5;2m";
            constexpr auto yellow   = "\033[38;5;3m";
            constexpr auto darkblue = "\033[38;5;4m";
            constexpr auto pink     = "\033[38;5;5m";
            constexpr auto blue     = "\033[38;5;6m";
            constexpr auto white    = "\033[38;5;7m";

            namespace strong {
                constexpr auto black    = "\033[38;5;8m";       // gray
                constexpr auto red      = "\033[38;5;9m";
                constexpr auto green    = "\033[38;5;10m";
                constexpr auto yellow   = "\033[38;5;11m";
                constexpr auto darkblue = "\033[38;5;12m";
                constexpr auto pink     = "\033[38;5;13m";
                constexpr auto blue     = "\033[38;5;14m";
                constexpr auto white    = "\033[38;5;15m";
            }
        } // namespace fg

        namespace bg {
            constexpr auto black    = "\033[48;5;0m";
            constexpr auto red      = "\033[48;5;1m";
            constexpr auto green    = "\033[48;5;2m";
            constexpr auto yellow   = "\033[48;5;3m";
            constexpr auto darkblue = "\033[48;5;4m";
            constexpr auto pink     = "\033[48;5;5m";
            constexpr auto blue     = "\033[48;5;6m";
            constexpr auto white    = "\033[48;5;7m";

            namespace strong {
                constexpr auto black    = "\033[48;5;8m";       // gray
                constexpr auto red      = "\033[48;5;9m";
                constexpr auto green    = "\033[48;5;10m";
                constexpr auto yellow   = "\033[48;5;11m";
                constexpr auto darkblue = "\033[48;5;12m";
                constexpr auto pink     = "\033[48;5;13m";
                constexpr auto blue     = "\033[48;5;14m";
                constexpr auto white    = "\033[48;5;15m";
            }
        } // namespace bg
    } // namespace color256

    namespace true_color {
        constexpr auto red = "\033[38;2;255;0;0m";
    }
}

namespace utl {

template <class T> requires std::is_fundamental_v<T>
[[nodiscard]] std::string toString(T x) {
    return std::to_string(x);
}

[[nodiscard]] std::string toString(const char* x) {
    return std::string(x);
}

[[nodiscard]] std::string toString(std::string_view x) {
    return std::string(x);
}

[[nodiscard]] std::string toString(const std::source_location x) {
     return x.file_name() + std::string(":") + std::to_string(x.line());
}

template <class T, class ...Ts>
[[nodiscard]] std::string joinStr(const std::string& separator, T&& first, Ts&&... args) {
    return toString(std::forward<T>(first)) +
        (... + (separator + toString(std::forward<Ts>(args))));
}

template <class ...Ts>
std::string colorize(const std::string& color, Ts&&... args) {
    return joinStr("", color, args..., term_colors::def);
}

template <class T, class ...Ts>
void print(const std::string& separator, T&& first, Ts&&... args) {
    std::cout << joinStr(separator, first, args...);
}

template <class T, class ...Ts>
void println(const std::string& separator, T&& first, Ts&&... args) {
    std::cout << joinStr(separator, first, args...) << '\n';
}

template <class T>
void println(T&& x) {
    std::cout << toString(x) << '\n';
}

} // namespace utl

template <class T>
concept generic_stringable = requires(const std::remove_reference_t<T>& x) {
    { utl::toString(x) };
};

template <class T>
concept user_stringable = requires(const std::remove_reference_t<T>& x) {
    { toString(x) };
};

template <class T>
concept stringable = generic_stringable<T> || user_stringable<T>;
