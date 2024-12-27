#pragma once

#include "nova/error.hh"

#include <fmt/core.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace nova {

using bytes = std::vector<std::byte>;

template <typename T>
concept binary_interpretable =
    std::is_same_v<T, char>
    || std::is_same_v<T, unsigned char>
    || std::is_same_v<T, std::uint8_t>
    || std::is_same_v<T, std::byte>;

enum class endian : std::uint8_t {
    big,
    little,
};

struct datacursor {
    std::size_t pos;
    std::size_t length;
    std::size_t size;
};

namespace detail {

/**
 * @brief   A binary data view on a range.
 *
 * Interprets binary data in a type safe manner either in big or little endian.
 *
 * NOTE: it is a non-owning view, it is the caller's responsibility to make sure
 * the `data_view` does not outlive the data it refers to.
 *
 * @throws  All accessor member functions throw if `RuntimeBoundCheck`
 *          is turned on, else calls to `assert` (debug mode check)
 */
template <endian Endianness = endian::big, bool RuntimeBoundCheck = true>
class data_view {
    static constexpr auto Byte = 8;

public:

    template <typename Range>
        requires binary_interpretable<std::remove_cv_t<typename Range::value_type>>
            and (
                std::contiguous_iterator<typename Range::iterator>
                or std::is_array_v<Range>
            )
    [[nodiscard]]
    data_view(const Range& data)
        : m_data(std::as_bytes(std::span(data)))
    {}

    [[nodiscard]]
    data_view(const void* ptr, std::size_t size)
        : m_data(reinterpret_cast<const std::byte*>(ptr), size)
    {}

    [[nodiscard]] data_view subview(std::size_t offset) const {
        return data_view(m_data.subspan(offset));
    }

    [[nodiscard]] data_view subview(std::size_t offset, std::size_t length) const {
        return data_view(m_data.subspan(offset, length));
    }

    [[nodiscard]] auto begin() const { return std::begin(m_data); }
    [[nodiscard]] auto begin()       { return std::begin(m_data); }
    [[nodiscard]] auto end()   const { return std::end(m_data); }
    [[nodiscard]] auto end()         { return std::end(m_data); }

    [[nodiscard]] auto span()        const -> std::span<const std::byte>  { return m_data; }
    [[nodiscard]] auto empty()       const -> bool                        { return m_data.empty(); }
    [[nodiscard]] auto size()        const -> std::size_t                 { return m_data.size(); }

    [[nodiscard]] auto ptr()         const -> const std::byte*  { return m_data.data(); }
    [[nodiscard]] auto char_ptr()    const -> const char*       { return reinterpret_cast<const char*>(m_data.data()); }
    [[nodiscard]] auto nc_char_ptr() const -> char*             { return const_cast<char*>(reinterpret_cast<const char*>(m_data.data())); }

    /**
     * @brief   Interpret data as number according to the type `T`.
     */
    template <typename T>
        requires std::is_integral_v<T>
    [[nodiscard]] auto as_number(std::size_t pos) const -> T {
        boundary_check(pos, sizeof(T));

        auto ret = T {};
        for (std::size_t i = 0; i < sizeof(T); ++i) {
            if constexpr (Endianness == endian::big) {
                ret |= static_cast<T>(std::to_integer<T>(m_data[pos + i]) << (sizeof(T) - i - 1) * Byte);
            }
            else {
                ret |= static_cast<T>(std::to_integer<T>(m_data[pos + i]) << i * Byte);
            }
        }
        return ret;
    }

    /**
     * @brief   Interpret data as a number, length specified dynamically.
     *
     * Convenience function for hiding switch cases.
     */
    [[nodiscard]]
    auto as_number(std::size_t pos, std::uint8_t length) const -> std::size_t {
        switch (length) {
            case 1:     return as_number<std::uint8_t>(pos);
            case 2:     return as_number<std::uint16_t>(pos);
            case 4:     return as_number<std::uint32_t>(pos);
        }
        throw std::domain_error("Invalid integer length");
    }

    /**
     * @brief   Interpret data for the given `length` as a string
     */
    [[nodiscard]]
    auto as_string(std::size_t pos, std::size_t length) const -> std::string_view {
        boundary_check(pos, length);
        return { reinterpret_cast<const char*>(m_data.data() + pos), length };
    }

    [[nodiscard]]
    auto as_string() const -> std::string_view {
        return as_string(0, size());
    }

    /**
     * @brief   Interpret data as dynamic length string.
     *
     * Length of the string is described in the first `length_bytes` bytes.
     */
    [[nodiscard]]
    auto as_dyn_string(std::size_t pos, std::uint8_t length_bytes = 1) const -> std::string_view {
        const auto str_length = as_number(pos, length_bytes);
        boundary_check(pos, str_length);
        return as_string(pos + length_bytes, str_length);
    }

    [[nodiscard]]
    auto to_hex() const {
        return spdlog::to_hex(m_data);
    }

    [[nodiscard]]
    auto as_hex_string(std::size_t pos, std::size_t length) const -> std::string {
        std::string ret;

        for (const auto& ch : m_data.subspan(pos, length)) {
            ret += fmt::format("{:02x}", ch);
        }

        return ret;
    }

    [[nodiscard]]
    auto as_hex_string() const -> std::string {
        return as_hex_string(0, size());
    }

    [[nodiscard]]
    auto to_vec() const -> bytes {
        auto ret = bytes(size());

        std::ranges::copy(
            std::begin(m_data),
            std::end(m_data),
            std::begin(ret)
        );

        return ret;
    }

private:
    std::span<const std::byte> m_data;

    void boundary_check(std::size_t pos, std::size_t length) const {
        if constexpr (RuntimeBoundCheck) {
            if (size() < pos + length) {
                throw exception<datacursor>("Out of bounds access", datacursor{ pos, length, size() });
            }
        }
        else {
            nova_assert(pos + length <= size());
        }
    }
};

} // namespace detail

using data_view = detail::data_view<>;
using data_view_be = detail::data_view<endian::big>;
using data_view_le = detail::data_view<endian::little>;

class serializer {
    static constexpr auto Byte = 8;

public:
    serializer() = default;
    serializer(std::size_t size)
        : m_data(size)
    {}

    template <typename T>
    void operator()(T x) {
        // TODO(safety): auto-resize if needed
        // TODO(refact): generic algorithm (diff binary size and perf)
        impl(x);
    }

    [[nodiscard]] auto data() const -> bytes {
        return m_data;
    }

private:
    bytes m_data;
    std::size_t m_offset = 0;

    void impl(std::uint8_t x) {
        if (m_offset >= m_data.size()) { THROWUP; }
        m_data[m_offset] = std::byte(x);
        m_offset += 1;
    }

    void impl(std::uint16_t x) {
        if (m_offset >= m_data.size()) { THROWUP; }
        m_data[m_offset    ] = std::byte((x & 0xFF00) >> 8);
        m_data[m_offset + 1] = std::byte( x & 0x00FF);
        m_offset += 2;
    }

    void impl(std::uint32_t x) {
        if (m_offset >= m_data.size()) { THROWUP; }
        m_data[m_offset    ] = std::byte((x & 0xFF000000) >> 24);
        m_data[m_offset + 1] = std::byte((x & 0x00FF0000) >> 16);
        m_data[m_offset + 2] = std::byte((x & 0x0000FF00) >>  8);
        m_data[m_offset + 3] = std::byte( x & 0x000000FF);
        m_offset += 4;
    }

    void impl(std::uint64_t x) {
        if (m_offset >= m_data.size()) { THROWUP; }
        m_data[m_offset    ] = std::byte((x & 0xFF000000'00000000) >> 56);
        m_data[m_offset + 1] = std::byte((x & 0x00FF0000'00000000) >> 48);
        m_data[m_offset + 2] = std::byte((x & 0x0000FF00'00000000) >> 40);
        m_data[m_offset + 3] = std::byte((x & 0x000000FF'00000000) >> 32);
        m_data[m_offset + 4] = std::byte((x & 0x00000000'FF000000) >> 24);
        m_data[m_offset + 5] = std::byte((x & 0x00000000'00FF0000) >> 16);
        m_data[m_offset + 6] = std::byte((x & 0x00000000'0000FF00) >>  8);
        m_data[m_offset + 7] = std::byte( x & 0x00000000'000000FF);
        m_offset += 8;
    }

    void impl(std::string_view x) {
        using DT = bytes::difference_type;
        std::ranges::copy(data_view{ x }, std::next(std::begin(m_data), static_cast<DT>(m_offset)));
    }

};

} // namespace nova

template <>
class fmt::formatter<nova::datacursor> {
public:
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template <typename FmtContext>
    constexpr auto format(const nova::datacursor& cursor, FmtContext& ctx) const {
        return fmt::format_to(
            ctx.out(),
            "Pos={} Len={} End={} (Size={})",
            cursor.pos,
            cursor.length,
            cursor.pos + cursor.length,
            cursor.size
        );
    }
};
