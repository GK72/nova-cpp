#pragma once

#include <chrono>
#include <type_traits>

namespace nova {

template <typename T>                    struct is_chrono_duration : std::false_type {};
template <typename Rep, typename Period> struct is_chrono_duration<std::chrono::duration<Rep, Period>> : std::true_type {};
template <typename T> inline constexpr bool is_chrono_duration_v = is_chrono_duration<T>::value;

template <typename T> concept arithmetic = std::is_arithmetic_v<T>;
template <typename T> concept floating_point = std::is_floating_point_v<T>;
template <typename T> concept chrono_duration = is_chrono_duration_v<T>;

} // namespace nova
