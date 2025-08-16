#pragma once

#include <libnova/vec.hpp>

#include <array>
#include <type_traits>

namespace nova {

enum class color_scale {
    Scaled,
    Normalized
};

template <color_scale Scale = color_scale::Normalized>
class color : public vec<4, float> {
private:
    struct scaled     { static constexpr float value = 255.0F; };
    struct non_scaled { static constexpr float value =   1.0F; };

public:
    using vec_type = vec<4, float>;
    using scaling_factor = std::conditional_t<Scale == color_scale::Scaled, scaled, non_scaled>;

    constexpr color() = default;

    constexpr color(float r, float g, float b, float a)
        : vec_type(std::to_array({ r, g, b, a }))
    {}

    constexpr color(int r, int g, int b, int a)
        : vec_type(std::to_array({
            static_cast<float>(r) / 255.0F,
            static_cast<float>(g) / 255.0F,
            static_cast<float>(b) / 255.0F,
            static_cast<float>(a) / 255.0F
        }))
    {}

    [[nodiscard]] constexpr float r() const noexcept { return vec_type::m_vec[0] * scaling_factor::value; }
    [[nodiscard]] constexpr float g() const noexcept { return vec_type::m_vec[1] * scaling_factor::value; }
    [[nodiscard]] constexpr float b() const noexcept { return vec_type::m_vec[2] * scaling_factor::value; }
    [[nodiscard]] constexpr float a() const noexcept { return vec_type::m_vec[3] * scaling_factor::value; }
};

using Color = color<>;

/**
 * @brief   Pack a color vector as 8-bit values into one 32 bit value (Big endian)
 */
template <color_scale Scale>
[[nodiscard]] constexpr std::uint32_t pack32BE(const color<Scale>& value) noexcept {
    return pack32BE(value.r(), value.g(), value.b(), value.a());
}

/**
 * @brief   Pack a color vector as 8-bit values into one 32 bit value (Little endian)
 */
template <color_scale Scale>
[[nodiscard]] constexpr std::uint32_t pack32LE(const color<Scale>& value) noexcept {
    return pack32BE(value.a(), value.b(), value.g(), value.r());
}

namespace colors {

    constexpr auto black = color{   0,   0,   0, 255 };
    constexpr auto white = color{ 255, 255, 255, 255 };
    constexpr auto red   = color{ 255,   0,   0, 255 };
    constexpr auto green = color{   0, 255,   0, 255 };
    constexpr auto blue  = color{   0,   0, 255, 255 };

    constexpr auto petrol_blue = color{ 0.1294F, 0.3922F, 0.4667F, 1.0F };

} // namespace colors

} // namespace nova
