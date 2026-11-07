/**
 * Part of Nova C++ Library.
 *
 * @brief   What the folding CRC-32 implementations share: the constants and
 *          the 128-bit steps.
 *
 * x86-64 only. A 128-bit block is replaced with its two 64-bit halves
 * multiplied by constants, which leaves the same remainder 128 bits or more
 * further on, where it is XOR-ed into the data.
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

#pragma once

#include <libnova/detail/cpu.hpp>

#if defined(NOVA_X86_64)

#include <immintrin.h>

#include <cstddef>
#include <cstdint>
#include <span>

namespace nova::detail::crc32_fold {

/// The generator polynomial with its x^32 term.
inline constexpr std::uint64_t P = 0x104C11DB7U;

/**
 * @brief   x^e mod P, bit-reversed into the 33 bits a reflected carry-less
 *          multiply needs.
 *
 * Multiplying by it moves a 64-bit half `e + 32` bits towards the end of the
 * data.
 */
consteval auto fold_constant(unsigned e) -> std::uint64_t {
    std::uint64_t r = 1;
    for (unsigned i = 0; i < e; ++i) {
        r <<= 1U;
        if ((r & (1ULL << 32U)) != 0) {
            r ^= P;
        }
    }

    std::uint64_t reflected = 0;
    for (unsigned i = 0; i < 32; ++i) {
        reflected |= ((r >> i) & 1U) << (31U - i);
    }
    return reflected << 1U;
}

/**
 * @brief   The pair that folds a 128-bit block `distance` bits forward: the low
 *          half travels 64 bits further than the high half.
 */
struct fold_pair {
    std::uint64_t lo;
    std::uint64_t hi;
};

consteval auto fold_by(unsigned distance) -> fold_pair {
    return { .lo = fold_constant(distance + 32), .hi = fold_constant(distance - 32) };
}

inline constexpr auto Fold16   = fold_by(128);
inline constexpr auto Fold32   = fold_by(256);
inline constexpr auto Fold48   = fold_by(384);
inline constexpr auto Fold64   = fold_by(512);
inline constexpr auto Fold256  = fold_by(2048);
inline constexpr auto Reduce64 = fold_constant(64);

consteval auto reflect33(std::uint64_t v) -> std::uint64_t {
    std::uint64_t ret = 0;
    for (unsigned i = 0; i < 33; ++i) {
        ret |= ((v >> i) & 1U) << (32U - i);
    }
    return ret;
}

/**
 * @brief   The quotient x^64 / P, carry-less
 *
 * See `doc/simd-guide.adoc#crc-barrett`.
 *
 * Long division, bringing down one bit of x^64 at a time.
 */
consteval auto x64_div_p() -> std::uint64_t {
    std::uint64_t rem = 0;
    std::uint64_t quotient = 0;

    for (int i = 64; i >= 0; --i) {
        rem = (rem << 1U) | (i == 64 ? 1U : 0U);
        if ((rem & (1ULL << 32U)) != 0) {
            rem ^= P;
            quotient |= 1ULL << static_cast<unsigned>(i);
        }
    }

    return quotient;
}

// Barrett reduction: x^64 / P, and P, both bit-reversed to 33 bits
inline constexpr std::uint64_t BarrettMu = reflect33(x64_div_p());
inline constexpr std::uint64_t BarrettP  = reflect33(P);
static_assert(BarrettMu == 0x1F7011641U && BarrettP == 0x1DB710641U);

// The constants of Intel's "Fast CRC Computation for Generic Polynomials Using
// PCLMULQDQ Instruction", as Linux and zlib use them
static_assert(Fold64.lo == 0x154442BD4U && Fold64.hi == 0x1C6E41596U);
static_assert(Fold16.lo == 0x1751997D0U && Fold16.hi == 0x0CCAA009EU);
static_assert(Reduce64 == 0x163CD6124U);

NOVA_TARGET("pclmul,sse4.1")
inline auto load(const std::byte* ptr) -> __m128i {
    return _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr));                                  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast) | The intrinsics' interface
}

NOVA_TARGET("pclmul,sse4.1")
inline auto constant(fold_pair k) -> __m128i {
    return _mm_set_epi64x(static_cast<long long>(k.hi), static_cast<long long>(k.lo));
}

/**
 * @brief   `x` folded `k` forward, onto `next`.
 */
NOVA_TARGET("pclmul,sse4.1")
inline auto fold(__m128i x, __m128i k, __m128i next) -> __m128i {
    return _mm_xor_si128(_mm_xor_si128(_mm_clmulepi64_si128(x, k, 0x00), _mm_clmulepi64_si128(x, k, 0x11)), next);
}

/**
 * @brief   Folds the 16-byte blocks of `data` into `x`, then reduces it to the
 *          register (`crc32_pclmul.cpp`).
 */
NOVA_TARGET("pclmul,sse4.1")
auto fold_tail_and_reduce(__m128i x, std::span<const std::byte> data) -> std::uint32_t;

} // namespace nova::detail::crc32_fold

#endif // NOVA_X86_64
