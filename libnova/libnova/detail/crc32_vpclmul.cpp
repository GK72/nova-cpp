/**
 * Part of Nova C++ Library.
 *
 * @brief   CRC-32 by folding with VPCLMULQDQ on AVX-512: four 512-bit
 *          registers, 256 bytes per step.
 *
 * A register is four blocks side by side; at the end its lanes fold into one
 * and the 128-bit steps of `crc32_fold.hpp` finish.
 *
 * See the SIMD Guide (`doc/simd-guide.adoc#crc-vpclmul`).
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

#define NOVA_TARGET_AVX512 NOVA_TARGET("avx512f,avx512vl,vpclmulqdq,pclmul,sse4.1")

namespace nova::detail {

namespace {

    using namespace crc32_fold;

    NOVA_TARGET_AVX512
    auto load512(const std::byte* ptr) -> __m512i {
        return _mm512_loadu_si512(ptr);
    }

    NOVA_TARGET_AVX512
    auto constant512(fold_pair k) -> __m512i {
        return _mm512_broadcast_i32x4(constant(k));
    }

    /// `x` folded `k` forward in each 128-bit lane, onto `next`.
    NOVA_TARGET_AVX512
    auto fold512(__m512i x, __m512i k, __m512i next) -> __m512i {
        constexpr int Xor3 = 0x96;
        return _mm512_ternarylogic_epi64(_mm512_clmulepi64_epi128(x, k, 0x00), _mm512_clmulepi64_epi128(x, k, 0x11), next, Xor3);
    }

    /// At least 256 bytes, a multiple of 16.
    NOVA_TARGET_AVX512
    auto fold_blocks(std::span<const std::byte> data, std::uint32_t reg) -> std::uint32_t {
        const auto* ptr = data.data();
        auto x0 = _mm512_xor_si512(load512(ptr), _mm512_zextsi128_si512(_mm_cvtsi32_si128(static_cast<int>(reg))));
        auto x1 = load512(ptr + 64);
        auto x2 = load512(ptr + 128);
        auto x3 = load512(ptr + 192);
        data = data.subspan(256);

        const auto k256 = constant512(Fold256);
        for (; data.size() >= 256; data = data.subspan(256)) {
            ptr = data.data();
            x0 = fold512(x0, k256, load512(ptr));
            x1 = fold512(x1, k256, load512(ptr + 64));
            x2 = fold512(x2, k256, load512(ptr + 128));
            x3 = fold512(x3, k256, load512(ptr + 192));
        }

        const auto k64 = constant512(Fold64);
        x0 = fold512(x0, k64, x1);
        x0 = fold512(x0, k64, x2);
        x0 = fold512(x0, k64, x3);
        for (; data.size() >= 64; data = data.subspan(64)) {
            x0 = fold512(x0, k64, load512(data.data()));
        }

        // Lane i (the oldest first) folds 48, 32, 16 and 0 bytes forward
        const auto lanes = _mm512_set_epi64(
            0, 0,
            static_cast<long long>(Fold16.hi), static_cast<long long>(Fold16.lo),
            static_cast<long long>(Fold32.hi), static_cast<long long>(Fold32.lo),
            static_cast<long long>(Fold48.hi), static_cast<long long>(Fold48.lo)
        );
        const auto folded = fold512(x0, lanes, _mm512_setzero_si512());
        auto x = _mm_xor_si128(_mm512_extracti32x4_epi32(folded, 0), _mm512_extracti32x4_epi32(folded, 1));
        x = _mm_xor_si128(x, _mm512_extracti32x4_epi32(folded, 2));
        x = _mm_xor_si128(x, _mm512_extracti32x4_epi32(x0, 3));
        return fold_tail_and_reduce(x, data);
    }

} // namespace

auto crc32_vpclmul(std::span<const std::byte> data, std::uint32_t reg) -> std::uint32_t {
    if (data.size() < 256) {
        return crc32_pclmul(data, reg);
    }
    const auto blocks = data.size() & ~std::size_t{ 15 };
    return crc32_slice8(data.subspan(blocks), fold_blocks(data.first(blocks), reg));
}

} // namespace nova::detail

#undef NOVA_TARGET_AVX512

#endif // NOVA_X86_64
