#pragma once

#include <string>
#include <string_view>
#include <iostream>         // TODO(C++20): use format lib
#include <source_location>
#include <type_traits>

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

template <class T, class ...Ts>
void print(const std::string& separator, T&& first, Ts&&... args) {
    std::cout << joinStr(separator, first, args...);
}

template <class T, class ...Ts>
void println(const std::string& separator, T&& first, Ts&&... args) {
    std::cout << joinStr(separator, first, args...) << '\n';
}

} // namespace utl
