#pragma once

#include <algorithm>
#include <concepts>
#include <numeric>
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

[[nodiscard]] auto accumulate(const auto& range, auto op) {
    using namespace std::literals::string_literals;
    return std::accumulate(std::begin(range), std::end(range), ""s, op);
}

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
[[nodiscard]] std::string colorize(const std::string& color, Ts&&... args) {
    return joinStr("", color, args..., term_colors::def);
}

[[nodiscard]] std::vector<std::string> strSplit(const std::string& str, std::string_view split) {
    std::vector<std::string> parts;
    size_t start = 0;
    size_t end = 0;

    while (end = str.find(split, end), end != std::string::npos) {
        parts.push_back(str.substr(start, end - start));
        end += split.size();
        start = end;
    }
    parts.push_back(str.substr(start));

    return parts;
}

[[nodiscard]] std::string repeat(std::string_view sv, int n) {
    std::string ret;
    std::ranges::generate_n(std::back_inserter(ret), n, [&sv]{ return sv.back(); });
    return ret;
}

[[nodiscard]] std::string&& replace_all(std::string&& str, std::string_view what, std::string_view with) {
    auto pos = str.find(what, 0);
    while (pos != std::string::npos) {
        str.replace(pos, what.size(), with);
        pos = str.find(what, pos);
    }
    return std::move(str);
}

[[nodiscard]] std::string indent(int n, const std::string& str) {
    return accumulate(
        strSplit(str, "\n"),
        [n](const auto& init, const auto& x) { return init + repeat(" ", n) + x; }
    );
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

} // namespace utl
