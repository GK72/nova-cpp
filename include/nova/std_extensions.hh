/**
 * Part of Nova C++ Library.
 *
 * C++ Standard extensions where new compilers or library are not available.
 */

#pragma once

#include <type_traits>

namespace nova {

template <typename Enum>
[[nodiscard]] constexpr std::underlying_type_t<Enum> to_underlying(Enum e) noexcept {
    return static_cast<std::underlying_type_t<Enum>>(e);
}

} // namespace nova
