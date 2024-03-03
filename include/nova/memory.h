#pragma once

#include <cstddef>
#include <memory_resource>
#include <vector>

namespace nova {

/**
 * @brief   A custom memory resource using monotonic buffer.
 * 
 * Until the given buffer has free capacity, there is no memory allocation
 * happening from an OS perspective, i.e., no call to `new`/`malloc`.
 * When the buffer is full, the underlying resource will automatically request
 * new memory from the OS.
 *
 * Keeps track of the memory usage.
*/
class mem : public std::pmr::monotonic_buffer_resource {
public:
    mem(std::vector<std::byte>& buffer)
        : std::pmr::monotonic_buffer_resource(buffer.data(), buffer.size())
        , m_capacity(buffer.size())
    {}

    ~mem() = default;

    /**
     * @brief   Release memory to be able to reuse the underlying buffer.
     */
    void release() {
        std::pmr::monotonic_buffer_resource::release();
        m_total_allocations = 0;
        m_total_allocated_bytes = 0;
    }

    [[nodiscard]] auto total_allocations()      const noexcept { return m_total_allocations; }
    [[nodiscard]] auto total_allocated_bytes()  const noexcept { return m_total_allocated_bytes; }
    [[nodiscard]] auto remaining_capacity()     const noexcept { return m_capacity - m_total_allocated_bytes; }

    /**
     * @brief   Return true if upstream resource is or has been used.
     * 
     * Due to being a monotonic resource, once the upstream is used, it is
     * currently in use.
     */
    [[nodiscard]] auto is_upstream_used()       const noexcept { return m_total_allocated_bytes > m_capacity; }

private:
    long m_total_allocated_bytes = 0;
    long m_total_allocations = 0;
    std::size_t m_capacity;

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

class mem_factory {
    using Buffer = std::pmr::vector<std::byte>;

public:
    mem_factory(std::size_t buffer_size) {
        new_buffer(buffer_size);
    }

    Buffer* new_buffer(std::size_t buffer_size) {
        return &m_buffers.emplace_back(Buffer(buffer_size));
    }

    void delete_buffer(Buffer* buffer) {
        m_buffers.remove_if([buffer](const auto& x) { return &x == buffer; });
    }

    auto& last_buffer() {
        return m_buffers.back();
    }

    template <typename T>
    auto vector() {
        return std::pmr::vector<T>(m_buffers.back().size() / sizeof(T));
    }

private:
    std::list<Buffer> m_buffers;
};

} // namespace nova