#pragma once

#include <cstddef>
#include <memory_resource>
#include <vector>

namespace nova {

/**
 * @brief   A custom memory resource using monotonic buffer.
 * 
 * Keeps track of the memory usage.
*/
class mem : public std::pmr::monotonic_buffer_resource {
public:
    mem(std::vector<std::byte>& buffer)
        : std::pmr::monotonic_buffer_resource(buffer.data(), buffer.size())
    {}

    ~mem() = default;

    void release() {
        std::pmr::monotonic_buffer_resource::release();
        m_total_allocations = 0;
        m_total_allocated_bytes = 0;
    }

    [[nodiscard]] std::size_t total_allocations()       const noexcept { return m_total_allocations; }
    [[nodiscard]] std::size_t total_allocated_bytes()   const noexcept { return m_total_allocated_bytes; }

private:
    std::size_t m_total_allocated_bytes = 0;
    std::size_t m_total_allocations = 0;

    [[nodiscard]] void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        void* ptr = std::pmr::monotonic_buffer_resource::do_allocate(bytes, alignment);

        ++m_total_allocations;
        m_total_allocated_bytes += bytes;
        return ptr;
    }

    void do_deallocate(void* ptr, std::size_t bytes, std::size_t alignment) noexcept override {
        std::pmr::monotonic_buffer_resource::do_deallocate(ptr, bytes, alignment);

        m_total_allocated_bytes -= bytes;
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return std::pmr::monotonic_buffer_resource::do_is_equal(other);
    }
};

} // namespace nova