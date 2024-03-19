/**
 * @brief   Part of Nova Graphics Library.
 */

#include <libnova/random.hpp>
#include <libnova/vec.hpp>

#include <fmt/format.h>

#include <concepts>
#include <cmath>
#include <numbers>
#include <variant>
#include <vector>

namespace {

    /**
     * @brief   Helpert function to generate a random float number in [0, 1)
     */
    auto random_number() {
        return static_cast<float>(nova::random().number());
    }

    /**
     * @brief   Helper function to iterate in 2D
     */
    void scan(const pt::image& img, const auto& func) {
        for (int y = 0; y < static_cast<int>(img.height()); ++y) {
            for (int x = 0; x < static_cast<int>(img.width()); ++x) {
                func(x, y);
            }
        }
    }

} // namespace

namespace nova::gfx {

/**
 * @brief   Algorithm to calculate the color
 */
auto ray_color(const ray& r, const std::vector<primitive>& primitives) -> nova::Color {
    for (const auto& elem : primitives) {
        const std::optional<hit_record> ret = std::visit([&r](auto& p) { return hit(p, r); }, elem);
        if (ret.has_value()) {
            return (
                nova::Color{
                    ret->normal.x(),
                    ret->normal.y(),
                    ret->normal.z(),
                    1.0F
                } + 1.0F
            ) * 0.5F;
        }
    }

    // Background

    // TODO: lerp
    const auto t = (nova::unit(r.direction).y() + 1.0F) * 0.5F;
    return nova::Color{ 1.0F, 1.0F, 1.0F, 1.0F } * (1.0F - t)
         + nova::Color{ 0.5F, 0.7F, 1.0F, 1.0F } * t;
}

/**
 * @brief   Ray sampling
 *
 * @param n     Number of samples
 * @param x     Coordinate x
 * @param y     Coordinate y
 */
nova::Color pathtracer::sample(int n, int x, int y) {
    const auto& w = m_image.width();
    const auto& h = m_image.height();

    nova::Color color {};

    float ran_x = n > 1 ? random_number() : 0.0F;
    float ran_y = n > 1 ? random_number() : 0.0F;

    for (int i = 0; i < n; ++i) {
        const auto u = (static_cast<float>(x) + ran_x) / (w - 1);
        const auto v = (static_cast<float>(y) + ran_y) / (h - 1);

        color += ray_color(m_cam.raycast(u, v), m_primitives);
    }

    return color / static_cast<float>(n);
}

void pathtracer::update() {
    scan(m_image, [this](int x, int y) {
        const auto color = sample(m_config.sampling, x, y);
        m_image.at(x, y) = nova::pack32LE(color);
    });
}

void pathtracer::stop() {
    m_render_thread.request_stop();
    rerun();
}

void pathtracer::rerun() {
    std::unique_lock lk(m_sync.mtx);
    m_is_dirty = true;
    m_sync.cv.notify_all();
}

/**
 * @brief   Render if there is work to do
 *
 * Work meaning there is a change in scene or camera and
 * the thread is not stopped
 *
 * Note: The function is not actually static; it cannot be a member function
 *       because the `stop_token` is passed as first argument
 */
void pathtracer::render_loop(std::stop_token token, pathtracer* self) {                             // NOLINT(performance-unnecessary-value-param): `stop_token` must be passed by value
    while (not token.stop_requested()) {
        std::unique_lock lk(self->m_sync.mtx);
        self->m_sync.cv.wait(lk, [self]() { return self->m_is_dirty; });
        self->update();
        self->m_is_dirty = false;
    }
}

} // namespace nova::gfx;
