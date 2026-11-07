/**
 * Part of Nova C++ Library.
 *
 * @brief   Choosing the CRC-32 implementation the CPU supports.
 *
 * See the SIMD Guide (`doc/simd-guide.adoc#runtime-dispatch`).
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-07
 * @updated     2026-11-07
 * @status      experimental
 * @reviewed    read
 * SPDX-License-Identifier: BSL-1.0
 */

#include <libnova/detail/crc32.hpp>
#include <libnova/detail/cpu.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace nova::detail {

namespace {

    auto supported() -> std::vector<crc32_backend> {
        std::vector<crc32_backend> ret{
            { .name = "bytewise", .fn = crc32_bytewise },
            { .name = "slice8",   .fn = crc32_slice8 },
        };

#if defined(NOVA_X86_64)
        const auto& cpu = detail::cpu();
        if (cpu.pclmul && cpu.sse4_1) {
            ret.push_back({ .name = "pclmul", .fn = crc32_pclmul });
        }
        if (cpu.pclmul && cpu.sse4_1 && cpu.avx512f && cpu.avx512vl && cpu.vpclmulqdq) {
            ret.push_back({ .name = "vpclmul", .fn = crc32_vpclmul });
        }
#endif
#if defined(NOVA_CRC32_ARM)
        if (detail::cpu().crc32) {
            ret.push_back({ .name = "arm", .fn = crc32_arm });
        }
#endif

        return ret;
    }

} // namespace

auto crc32_backends() -> std::span<const crc32_backend> {
    static const auto backends = supported();
    return backends;
}

auto crc32_update(std::span<const std::byte> data, std::uint32_t reg) -> std::uint32_t {
    static const auto fastest = crc32_backends().back().fn;
    return fastest(data, reg);
}

} // namespace nova::detail
