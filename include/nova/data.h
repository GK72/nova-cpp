#pragma once

#include "nova/error.h"

#include <fmt/core.h>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/bin_to_hex.h>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace nova {

template <typename T>
concept binary_interpretable =
    std::is_same_v<T, char>
    || std::is_same_v<T, unsigned char>
    || std::is_same_v<T, std::uint8_t>
    || std::is_same_v<T, std::byte>;

enum class endian {
    big,
    little,
};

class out_of_data_bounds : public std::out_of_range {
public:
    out_of_data_bounds(std::size_t pos, std::size_t length, std::size_t size)
        : std::out_of_range(
            fmt::format("Pos: {}, Len: {}, Size: {} (End: {})", pos, length, size, pos + length)
        )
    {}
};

namespace detail {

/**
 * @brief   A binary data view on a range
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
    [[nodiscard]] explicit
    data_view(std::string_view data)
        : m_data(std::as_bytes(std::span(data)))
    {}

    template <typename T, std::size_t N>
    [[nodiscard]] explicit
    data_view(const std::array<T, N>& data)
        : m_data(std::as_bytes(std::span<const T, N>(data)))
    {}

    template <binary_interpretable T>
    [[nodiscard]]
    data_view(std::span<const T> data)
        : m_data(std::as_bytes(data))
    {}

    template <binary_interpretable T>
    [[nodiscard]]
    data_view(std::span<T> data)
        : m_data(std::as_bytes(data))
    {}

    [[nodiscard]] data_view subview(std::size_t offset) {
        return data_view(m_data.subspan(offset));
    }

    [[nodiscard]] std::size_t size() const {
        return m_data.size();
    }

    /**
     * @brief   Interpret data as number according to the type `T`
     */
    template <typename T>
        requires std::is_integral_v<T>
    [[nodiscard]] T as_number(std::size_t pos) const {
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
     * @brief   Interpret data as a number, length specified dynamically
     *
     * Convenience function for hiding switch cases.
     */
    [[nodiscard]]
    std::size_t as_number(std::size_t pos, std::uint8_t length) const {
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
    std::string_view as_string(std::size_t pos, std::size_t length) const {
        boundary_check(pos, length);
        return { reinterpret_cast<const char*>(m_data.data() + pos), length };
    }

    /**
     * @brief   Interpret data as dynamic length string
     *
     * Length of the string is described in the first `length_bytes` bytes.
     */
    [[nodiscard]]
    std::string_view as_dyn_string(std::size_t pos, std::uint8_t length_bytes = 1) const {
        const auto str_length = as_number(pos, length_bytes);
        boundary_check(pos, str_length);
        return as_string(pos + length_bytes, str_length);
    }

    [[nodiscard]]
    std::string as_hex_string(std::size_t pos, std::size_t length) const {
        boundary_check(pos, length);

        std::string ret;

        for (const auto& ch : m_data.subspan(pos, length)) {
            ret += fmt::format("{:02x}", ch);
        }

        return ret;
    }

    [[nodiscard]]
    std::string as_hex_string() const {
        return as_hex_string(0, size());
    }

    [[nodiscard]]
    std::string as_hex_string(std::size_t pos) const {
        return as_hex_string(pos, size() - pos);
    }

    [[nodiscard]]
    auto to_hex() const {
        return spdlog::to_hex(m_data);
    }

private:
    std::span<const std::byte> m_data;

    void boundary_check(std::size_t pos, std::size_t length) const {
        if constexpr (RuntimeBoundCheck) {
            if (size() < pos + length) {
                throw out_of_data_bounds(pos, length, size());
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
