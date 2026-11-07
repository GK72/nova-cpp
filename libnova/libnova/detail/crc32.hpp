/**
 * Part of Nova C++ Library.
 *
 * @brief   The CRC-32 implementations behind `nova::crc32()`.
 *
 * Each computes the same division on the inverted register (the state between
 * the inversions of `crc32()`), takes any length, and lives in its own
 * translation unit. One that needs an instruction set extension must only be
 * called when `crc32_backends()` lists it.
 *
 * See the SIMD Guide (`doc/simd-guide.adoc#crc32`).
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-07
 * @updated     2026-11-07
 * @status      experimental
 * @reviewed    read
 * SPDX-License-Identifier: BSL-1.0
 */

#pragma once

#include <libnova/detail/cpu.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace nova::detail {

using crc32_fn = auto (*)(std::span<const std::byte> data, std::uint32_t reg) -> std::uint32_t;

/**
 * @brief   One implementation of CRC-32.
 */
struct crc32_backend {
    std::string_view name;
    crc32_fn fn;
};

/**
 * @brief   The implementations the CPU supports.
 *
 * From the slowest to the fastest; `crc32_update()` uses the last.
 */
[[nodiscard]] auto crc32_backends() -> std::span<const crc32_backend>;

/**
 * @brief   The fastest implementation the CPU supports.
 */
[[nodiscard]] auto crc32_update(std::span<const std::byte> data, std::uint32_t reg) -> std::uint32_t;

/**
 * @brief   CRC32 one byte at a time.
 */
[[nodiscard]] auto crc32_bytewise(std::span<const std::byte> data, std::uint32_t reg) -> std::uint32_t;

/**
 * @brief   CRC32 eight bytes at a time.
 */
[[nodiscard]] auto crc32_slice8(std::span<const std::byte> data, std::uint32_t reg) -> std::uint32_t;

#if defined(NOVA_X86_64)

/**
 * @brief   CRC32 with folding with PCLMULQDQ (64 bytes per step).
 */
[[nodiscard]] auto crc32_pclmul(std::span<const std::byte> data, std::uint32_t reg) -> std::uint32_t;

/**
 * @brief   CRC32 with folding with VPCLMULQDQ on AVX-512, (256 bytes per step).
 */
[[nodiscard]] auto crc32_vpclmul(std::span<const std::byte> data, std::uint32_t reg) -> std::uint32_t;

#endif // NOVA_X86_64

#if defined(NOVA_ARM64) && defined(__ARM_FEATURE_CRC32)
    #define NOVA_CRC32_ARM

/**
 * @brief   The ARMv8 CRC32 instructions (eight bytes per instruction).
 */
[[nodiscard]] auto crc32_arm(std::span<const std::byte> data, std::uint32_t reg) -> std::uint32_t;

#endif

} // namespace nova::detail
