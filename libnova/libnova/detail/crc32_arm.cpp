/**
 * Part of Nova C++ Library.
 *
 * @brief   CRC-32 with the ARMv8 CRC32 instructions, eight bytes per
 *          instruction.
 *
 * Built only when the compiler enables the instructions for the whole build
 * (`__ARM_FEATURE_CRC32`).
 *
 * See the SIMD Guide (`doc/simd-guide.adoc#crc-arm`).
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-07
 * @updated     2026-11-07
 * @status      experimental
 * @reviewed    skimmed
 * SPDX-License-Identifier: BSL-1.0
 */

#include <libnova/detail/crc32.hpp>

#if defined(NOVA_CRC32_ARM)

#include <arm_acle.h>

#include <cstddef>
#include <cstdint>
#include <span>

namespace nova::detail {

auto crc32_arm(std::span<const std::byte> data, std::uint32_t reg) -> std::uint32_t {
    std::size_t i = 0;
    for (; i + 8 <= data.size(); i += 8) {
        std::uint64_t word = 0;
        for (std::size_t b = 0; b < 8; ++b) {
            word |= static_cast<std::uint64_t>(data[i + b]) << (8U * b);
        }
        reg = __crc32d(reg, word);
    }
    for (; i < data.size(); ++i) {
        reg = __crc32b(reg, std::to_integer<std::uint8_t>(data[i]));
    }
    return reg;
}

} // namespace nova::detail

#endif // NOVA_CRC32_ARM
