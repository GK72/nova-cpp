/**
 * Part of Nova C++ Library.
 *
 * @brief   CRC-32 by folding with PCLMULQDQ: four 128-bit blocks, 64 bytes per
 *          step.
 *
 * See the SIMD Guide (`doc/simd-guide.adoc#crc-folding`).
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-07
 * @updated     2026-11-07
 * @status      experimental
 * @reviewed    skimmed
 * SPDX-License-Identifier: BSL-1.0
 */

#include <libnova/detail/crc32.hpp>
#include <libnova/detail/crc32_fold.hpp>

#if defined(NOVA_X86_64)

#include <cstddef>
#include <cstdint>
#include <span>

namespace nova::detail {

namespace crc32_fold {

NOVA_TARGET("pclmul,sse4.1")
auto fold_tail_and_reduce(__m128i x, std::span<const std::byte> data) -> std::uint32_t {
    const auto k16 = constant(Fold16);
    for (; data.size() >= 16; data = data.subspan(16)) {
        x = fold(x, k16, load(data.data()));
    }

    // 128 to 64 bits: the low half folded onto the high half; this also
    // appends the 32 zero bits that make room for the remainder
    const auto low32 = _mm_setr_epi32(-1, 0, -1, 0);
    x = _mm_xor_si128(_mm_srli_si128(x, 8), _mm_clmulepi64_si128(x, k16, 0x10));
    x = _mm_xor_si128(_mm_srli_si128(x, 4),
                      _mm_clmulepi64_si128(_mm_and_si128(x, low32), _mm_set_epi64x(0, static_cast<long long>(Reduce64)), 0x00));

    // 64 to 32 bits: quotient = (low 32 bits * mu), remainder = x - quotient * P
    const auto barrett = _mm_set_epi64x(static_cast<long long>(BarrettMu), static_cast<long long>(BarrettP));
    auto q = _mm_clmulepi64_si128(_mm_and_si128(x, low32), barrett, 0x10);
    q = _mm_clmulepi64_si128(_mm_and_si128(q, low32), barrett, 0x00);
    return static_cast<std::uint32_t>(_mm_extract_epi32(_mm_xor_si128(x, q), 1));
}

} // namespace crc32_fold

namespace {

    using namespace crc32_fold;

    /// At least 64 bytes, a multiple of 16.
    NOVA_TARGET("pclmul,sse4.1")
    auto fold_blocks(std::span<const std::byte> data, std::uint32_t reg) -> std::uint32_t {
        const auto* ptr = data.data();
        auto x0 = _mm_xor_si128(load(ptr), _mm_cvtsi32_si128(static_cast<int>(reg)));
        auto x1 = load(ptr + 16);
        auto x2 = load(ptr + 32);
        auto x3 = load(ptr + 48);
        data = data.subspan(64);

        const auto k64 = constant(Fold64);
        for (; data.size() >= 64; data = data.subspan(64)) {
            ptr = data.data();
            x0 = fold(x0, k64, load(ptr));
            x1 = fold(x1, k64, load(ptr + 16));
            x2 = fold(x2, k64, load(ptr + 32));
            x3 = fold(x3, k64, load(ptr + 48));
        }

        const auto k16 = constant(Fold16);
        x0 = fold(x0, k16, x1);
        x0 = fold(x0, k16, x2);
        x0 = fold(x0, k16, x3);
        return fold_tail_and_reduce(x0, data);
    }

} // namespace

auto crc32_pclmul(std::span<const std::byte> data, std::uint32_t reg) -> std::uint32_t {
    if (data.size() < 64) {
        return crc32_slice8(data, reg);
    }
    const auto blocks = data.size() & ~std::size_t{ 15 };
    return crc32_slice8(data.subspan(blocks), fold_blocks(data.first(blocks), reg));
}

} // namespace nova::detail

#endif // NOVA_X86_64
