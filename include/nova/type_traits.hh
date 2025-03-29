/**
 * Part of Nova C++ Library.
 *
 * Metaprogramming utilities.
 */

#pragma once

#include <array>
#include <chrono>
#include <map>
#include <string_view>
#include <type_traits>
#include <vector>

namespace nova {

// Dependent false
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1830r1.pdf
//
// static_assert(false)
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2593r1.html
template <bool value, typename... Args>
inline constexpr bool dependent_bool_value = value;

template <typename... Args>
inline constexpr bool dependent_false = dependent_bool_value<false, Args...>;


template <typename T>                struct is_std_array : std::false_type {};
template <typename T, std::size_t N> struct is_std_array<std::array<T, N>> : std::true_type {};
template <typename T> inline constexpr bool is_std_array_v = is_std_array<T>::value;

template <typename T>                    struct is_chrono_duration : std::false_type {};
template <typename Rep, typename Period> struct is_chrono_duration<std::chrono::duration<Rep, Period>> : std::true_type {};
template <typename T> inline constexpr bool is_chrono_duration_v = is_chrono_duration<T>::value;

template <typename T> struct is_std_vector : std::false_type {};
template <typename T> struct is_std_vector<std::vector<T>> : std::true_type {};
template <typename T> inline constexpr bool is_std_vector_v = is_std_vector<T>::value;

template <typename T>             struct is_std_map : std::false_type {};
template <typename K, typename V> struct is_std_map<std::map<K, V>> : std::true_type {};
template <typename T> inline constexpr bool is_std_map_v = is_std_map<T>::value;


template <typename T> concept array_like = is_std_array_v<T>;
template <typename T> concept arithmetic = std::is_arithmetic_v<T>;
template <typename T> concept floating_point = std::is_floating_point_v<T>;
template <typename T> concept chrono_duration = is_chrono_duration_v<T>;
template <typename T> concept vector_like = is_std_vector_v<T>;
template <typename T> concept map_like = is_std_map_v<T>;
template <typename T> concept string_like = std::is_convertible_v<T, std::string_view>;

} // namespace nova
