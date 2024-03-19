/**
 * @brief   Part of Nova library.
 */

#pragma once

#include <libnova/vec.hpp>

namespace nova::gfx {

struct ray {
    nova::Vec3f origin;
    nova::Vec3f direction;

    constexpr nova::Vec3f at(float distance) const {
        return origin + direction * distance;
    }
};

class camera {
public:
    camera(nova::Vec2f dimensions) {
        const auto aspect_ratio = dimensions.x() / dimensions.y();
        const auto viewport_height = 2.0F;
        const auto viewport_width = aspect_ratio * viewport_height;

        m_horizontal  = nova::Vec3f{ viewport_width,  0.0F, 0.0F };
        m_vertical    = nova::Vec3f{ 0.0F, viewport_height, 0.0F };
        bottom_left_calc();
    }

    auto& x() { auto& ret = m_origin.x(); bottom_left_calc(); return ret; };
    auto& y() { auto& ret = m_origin.y(); bottom_left_calc(); return ret; };
    auto& z() { auto& ret = m_origin.z(); bottom_left_calc(); return ret; };

    void bottom_left_calc() {
        m_bottom_left = m_origin
            - m_horizontal / 2.0F
            - m_vertical / 2.0F
            - nova::Vec3f{ 0.0F, 0.0F, m_focal_length };
    }

    auto& focal_length() { auto& ret = m_focal_length; bottom_left_calc(); return ret; }

    ray raycast(float u, float v) const noexcept {
        return {
            m_origin,
            m_bottom_left
            + m_horizontal * u
            + m_vertical * v
            - m_origin
        };
    }

private:
    float m_focal_length = 10.0F;

    nova::Vec3f m_origin { 0.0F, 0.0F, m_focal_length };
    nova::Vec3f m_horizontal;
    nova::Vec3f m_vertical;
    nova::Vec3f m_bottom_left;
};

} // nova::gfx
