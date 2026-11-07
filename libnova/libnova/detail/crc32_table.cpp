/**
 * Part of Nova C++ Library.
 *
 * @brief   Table-driven CRC-32: byte-wise and slicing-by-8.
 *
 * Portable, the fallback of every CPU.
 *
 * See the SIMD Guide (`doc/simd-guide.adoc#crc-bytewise`, `#crc-slicing`).
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-07
 * @updated     2026-11-07
 * @status      experimental
 * @reviewed    skimmed
 * SPDX-License-Identifier: BSL-1.0
 */

#include <libnova/detail/crc32.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace nova::detail {

namespace {

    /// The generator polynomial 0x04C11DB7, bit-reversed.
    constexpr std::uint32_t Polynomial = 0xEDB88320U;

    /**
     * @brief   `[k][b]`: the register after byte `b` and then `k` zero bytes
     *          enter an empty register.
     *
     * Slice 0 is the byte-wise table; slice `k` runs one more byte step on
     * slice `k - 1`.
     */
    constexpr auto Tables = []() {
        std::array<std::array<std::uint32_t, 256>, 8> t {};

        for (std::uint32_t i = 0; i < 256; ++i) {
            std::uint32_t c = i;
            for (int k = 0; k < 8; ++k) {
                c = (c & 1U) != 0 ? Polynomial ^ (c >> 1U) : c >> 1U;
            }
            t[0][i] = c;
        }

        for (std::size_t k = 1; k < t.size(); ++k) {
            for (std::size_t i = 0; i < 256; ++i) {
                t[k][i] = t[0][t[k - 1][i] & 0xFFU] ^ (t[k - 1][i] >> 8U);
            }
        }

        return t;
    }();

    [[nodiscard]] auto byte_at(std::span<const std::byte> data, std::size_t i) -> std::uint32_t {
        return std::to_integer<std::uint32_t>(data[i]);
    }

} // namespace

auto crc32_bytewise(std::span<const std::byte> data, std::uint32_t reg) -> std::uint32_t {
    const auto& table = Tables[0];

    for (const auto b : data) {
        reg = table[(reg ^ std::to_integer<std::uint32_t>(b)) & 0xFFU] ^ (reg >> 8U);
    }

    return reg;
}

/**
 * @brief   Eight independent lookups per step, one per byte, in the slice of as many
 * zero bytes as follow it within the eight.
 */
auto crc32_slice8(std::span<const std::byte> data, std::uint32_t reg) -> std::uint32_t {
    const auto& t = Tables;

    std::size_t i = 0;
    for (; i + 8 <= data.size(); i += 8) {
        reg ^= byte_at(data, i)
            | (byte_at(data, i + 1) << 8U)
            | (byte_at(data, i + 2) << 16U)
            | (byte_at(data, i + 3) << 24U);

        reg = t[7][reg & 0xFFU]
            ^ t[6][(reg >> 8U) & 0xFFU]
            ^ t[5][(reg >> 16U) & 0xFFU]
            ^ t[4][reg >> 24U]
            ^ t[3][byte_at(data, i + 4)]
            ^ t[2][byte_at(data, i + 5)]
            ^ t[1][byte_at(data, i + 6)]
            ^ t[0][byte_at(data, i + 7)];
    }

    return crc32_bytewise(data.subspan(i), reg);
}

} // namespace nova::detail
