/**
 * Part of Nova C++ Library.
 *
 * @brief   Ring buffer.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2024-10-27
 * @updated     2026-11-07
 * @status      evolving
 * @reviewed    read
 * SPDX-License-Identifier: BSL-1.0
 */

#pragma once

#include <libnova/error.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

namespace nova {

/**
 * @brief   A ring buffer.
 *
 * Iteration visits the written elements from the oldest to the newest. The
 * storage (`data()`, `size()`) is circular: `offset()` is the next slot to
 * write, which holds the oldest element once the ring is full. Slots that were
 * never written hold `T{}`.
 */
template <typename T>
class ring {
    template <typename Value>
    class iterator_t {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = std::remove_const_t<Value>;
        using difference_type   = std::ptrdiff_t;
        using pointer           = Value*;
        using reference         = Value&;

        iterator_t() = default;

        iterator_t(Value* data, std::size_t size, std::size_t first, std::size_t pos)
            : m_data(data)
            , m_size(size)
            , m_first(first)
            , m_pos(pos)
        {}

        [[nodiscard]] auto operator*()  const -> reference { return m_data[(m_first + m_pos) % m_size]; }
        [[nodiscard]] auto operator->() const -> pointer   { return &**this; }

        auto operator++() -> iterator_t& {
            ++m_pos;
            return *this;
        }

        auto operator++(int) -> iterator_t {
            auto ret = *this;
            ++m_pos;
            return ret;
        }

        [[nodiscard]] auto operator==(const iterator_t&) const -> bool = default;

    private:
        Value* m_data = nullptr;
        std::size_t m_size = 0;
        std::size_t m_first = 0;
        std::size_t m_pos = 0;

    };

public:
    using iterator       = iterator_t<T>;
    using const_iterator = iterator_t<const T>;

    explicit ring(std::size_t size)
        : m_data(size)
    {}

    /**
     * @brief   Overwrite the oldest element once the ring is full; a ring of size 0 keeps nothing.
     */
    void push_back(const T& item) {
        if (m_data.empty()) {
            return;
        }

        m_data[m_idx] = item;
        m_idx = (m_idx + 1) % size();
        m_count = std::min(m_count + 1, size());
    }

    [[nodiscard]] auto begin()       -> iterator       { return { m_data.data(), size(), oldest(), 0 }; }
    [[nodiscard]] auto begin() const -> const_iterator { return { m_data.data(), size(), oldest(), 0 }; }
    [[nodiscard]] auto end()         -> iterator       { return { m_data.data(), size(), oldest(), m_count }; }
    [[nodiscard]] auto end()   const -> const_iterator { return { m_data.data(), size(), oldest(), m_count }; }

    [[nodiscard]] constexpr const T* data()      const noexcept { return m_data.data(); }
    [[nodiscard]] constexpr std::size_t size()   const noexcept { return m_data.size(); }
    [[nodiscard]] constexpr std::size_t offset() const noexcept { return m_idx; }

    [[nodiscard]] constexpr const T& back() const {
        nova_assert(size() > 0);
        return m_idx == 0 ? m_data[size() - 1] : m_data[m_idx - 1];
    }

    /**
     * @brief   Change the size, keeping the newest elements from the oldest to the newest at the
     *          front of the storage.
     */
    constexpr void resize(std::size_t size) {
        const auto keep = std::min(size, m_count);
        const auto first = oldest();

        auto ret = std::vector<T>{};
        ret.reserve(size);

        for (auto i = m_count - keep; i < m_count; ++i) {
            ret.push_back(std::move(m_data[(first + i) % m_data.size()]));
        }

        ret.resize(size);
        m_data = std::move(ret);
        m_idx = keep == size ? 0 : keep;
        m_count = keep;
    }

    /**
     * @brief   Remove every element; the size stays.
     */
    constexpr void clear() {
        std::ranges::fill(m_data, T{});
        m_idx = 0;
        m_count = 0;
    }

private:
    std::vector<T> m_data;
    std::size_t m_idx = 0;
    std::size_t m_count = 0;        ///< Written slots, at most `size()`

    /**
     * @brief   Slot of the oldest element.
     */
    [[nodiscard]] auto oldest() const -> std::size_t {
        return m_count == m_data.size() ? m_idx : 0;
    }

};

} // namespace nova
