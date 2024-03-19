/**
 * @brief   Part of Nova Graphics Library.
 */

#pragma once

#include <libnova/color.hpp>
#include <libnova-gfx/camera.hpp>
#include <libnova-gfx/primitives.hpp>

#include <thread>
#include <vector>

namespace nova::gfx {

class image {
public:
    image(nova::Vec2f dimensions)
        : m_dimensions(dimensions)
        , m_data(
            static_cast<std::size_t>(area(dimensions)),
            nova::pack32LE(nova::colors::black)
        )
    {}

    image(const image&) = delete;
    image(image&&) = delete;
    image& operator=(const image&) = delete;
    image& operator=(image&&) = delete;

    [[nodiscard]] const auto* data() const noexcept { return m_data.data(); }
    [[nodiscard]] auto* data()             noexcept { return m_data.data(); }
    [[nodiscard]] auto width()       const noexcept { return m_dimensions.x(); }
    [[nodiscard]] auto height()      const noexcept { return m_dimensions.y(); }
    [[nodiscard]] auto size()        const noexcept { return m_data.size(); }
    [[nodiscard]] const auto& vec()  const noexcept { return m_data; }
    [[nodiscard]] auto dimensions()  const noexcept { return m_dimensions; }
    [[nodiscard]] auto& texture_id()       noexcept { return m_texture_id; }

    [[nodiscard]] auto& at(int x, int y) noexcept {
        using SizeT = decltype(m_data)::size_type;
        return m_data[static_cast<SizeT>(y * static_cast<int>(width()) + x)];
    }

private:
    nova::Vec2f m_dimensions;
    std::vector<std::uint32_t> m_data;
    unsigned int m_texture_id;
};


class pathtracer {
public:
    struct config {
        int sampling = 1;
    };

    pathtracer(image& img, const camera& cam, const std::vector<primitive>& primitives)
        : m_image(img)
        , m_cam(cam)
        , m_primitives(primitives)
        , m_render_thread(&pathtracer::render_loop, this)
    {}

    auto& config()      { return m_config; }
    void rerun();
    void stop();
    void update();

private:
    image& m_image;
    const camera& m_cam;
    const std::vector<primitive>& m_primitives;
    std::jthread m_render_thread;

    bool m_is_dirty = true;
    struct config m_config;

    nova::Color sample(int n, int x, int y);

    static void render_loop(pathtracer* self);
};

} // namespace nova::gfx
