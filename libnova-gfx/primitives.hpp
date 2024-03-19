/**
 * @brief   Part of Nova Graphics Library.
 */

#pragma once

#include <libnova/color.hpp>
#include <libnova/vec.hpp>

#include <variant>

namespace nova::gfx {

struct hit_record {
    nova::Vec3f point;
    nova::Vec3f normal;
    float t;
};

struct sphere {
    nova::Vec3f position;
    float radius;
    nova::Color color;
};

inline std::optional<hit_record> hit(sphere sphere, const ray& r) {
    const auto x = r.origin - sphere.position;
    const auto a = nova::dot(r.direction, r.direction);
    const auto b = nova::dot(x, r.direction) * 2.0F;
    const auto c = nova::dot(x, x) - sphere.radius * sphere.radius;
    const auto discriminant = b * b - 4 * a * c;

    if (discriminant < 0) {
        return std::nullopt;
    }

    hit_record ret;
    ret.t = (-b - std::sqrt(discriminant)) / (2.0F * a);
    ret.point = r.at(ret.t),
    ret.normal = (ret.point - sphere.position) / sphere.radius;

    return ret;
}

using primitive = std::variant<sphere>;

} // namespace nova::gfx
