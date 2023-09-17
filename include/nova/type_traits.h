#pragma once

namespace nova {

template <typename T> concept arithmetic = std::is_arithmetic_v<T>;
template <typename T> concept floating_point = std::is_floating_point_v<T>;

} // namespace nova
