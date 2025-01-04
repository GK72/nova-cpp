#pragma once

#include "nova/error.hh"

#include <fmt/core.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace nova::detail {

    struct datacursor {
        std::size_t pos;
        std::size_t length;
        std::size_t size;
    };

} // namespace nova::detail

template <>
class fmt::formatter<nova::detail::datacursor> {
public:
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template <typename FmtContext>
    constexpr auto format(const nova::detail::datacursor& cursor, FmtContext& ctx) const {
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

    [[nodiscard]] auto span()  const -> std::span<const std::byte> { return m_data; }
    [[nodiscard]] auto empty() const -> bool                       { return m_data.empty(); }
    [[nodiscard]] auto size()  const -> std::size_t                { return m_data.size(); }

    [[nodiscard]] auto ptr()      const -> const std::byte* { return m_data.data(); }
    [[nodiscard]] auto char_ptr() const -> const char*      { return reinterpret_cast<const char*>(m_data.data()); }

    /**
     * @brief   Interpret data as number according to the type `T`.
     */
    template <typename T = std::size_t>
        requires std::is_integral_v<T>
    [[nodiscard]] auto as_number(std::size_t pos, std::size_t length) const -> T {
        boundary_check(pos, length);

        auto ret = T {};
        for (std::size_t i = 0; i < length; ++i) {
            if constexpr (Endianness == endian::big) {
                ret |= static_cast<T>(std::to_integer<T>(m_data[pos + i]) << (length - i - 1) * Byte);
            }
            else {
                ret |= static_cast<T>(std::to_integer<T>(m_data[pos + i]) << i * Byte);
            }
        }
        return ret;
    }

    /**
     * @brief   Interpret data as number according to the type `T`.
     */
    template <typename T>
        requires std::is_integral_v<T>
    [[nodiscard]] auto as_number(std::size_t pos) const -> T {
        return as_number<T>(pos, sizeof(T));
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
                throw exception("Out of bounds access: {}", detail::datacursor{ pos, length, size() });
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

} // namespace nova
