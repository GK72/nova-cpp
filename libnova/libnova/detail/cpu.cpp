/**
 * Part of Nova C++ Library.
 *
 * @brief   Detecting the CPU's instruction set extensions.
 *
 * x86-64 reports the CPU's extensions with `cpuid`, and the register state the
 * OS saves on a context switch with `xgetbv` (XCR0): an extension whose
 * registers the OS does not save is unusable.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-07
 * @updated     2026-11-07
 * @status      experimental
 * @reviewed    skimmed
 * SPDX-License-Identifier: BSL-1.0
 */

#include <libnova/detail/cpu.hpp>

#include <array>
#include <cstddef>
#include <cstdint>

#if defined(NOVA_X86_64)
    #if defined(NOVA_MSVC)
        #include <immintrin.h>
        #include <intrin.h>
    #else
        #include <cpuid.h>
    #endif
#endif

namespace nova::detail {

namespace {

#if defined(NOVA_X86_64)

    // Indices of the registers `cpuid` returns
    constexpr std::size_t eax = 0;
    constexpr std::size_t ebx = 1;
    constexpr std::size_t ecx = 2;
    constexpr std::size_t edx = 3;

    /**
     * @brief   Queries the CPU: `leaf` (and `subleaf`) select what it reports in
     *          the four registers, indexed by `eax`, `ebx`, `ecx` and `edx`.
     *
     * Leaf 0 reports the highest leaf, so a leaf above it must not be read.
     * See the SIMD Guide (`doc/simd-guide.adoc#x86-detection`).
     */
    auto cpuid(std::uint32_t leaf, std::uint32_t subleaf) -> std::array<std::uint32_t, 4> {
        std::array<std::uint32_t, 4> regs {};
#if defined(NOVA_MSVC)
        std::array<int, 4> r {};
        __cpuidex(r.data(), static_cast<int>(leaf), static_cast<int>(subleaf));
        for (std::size_t i = 0; i < r.size(); ++i) {
            regs[i] = static_cast<std::uint32_t>(r[i]);
        }
#else
        __cpuid_count(leaf, subleaf, regs[eax], regs[ebx], regs[ecx], regs[edx]);
#endif
        return regs;
    }

    /**
     * @brief   One feature flag of `cpuid`: bit `bit` of register `reg` of the
     *          `leaf`/`subleaf` query.
     */
    struct cpuid_flag {
        std::uint32_t leaf;
        std::uint32_t subleaf;
        std::size_t reg;
        unsigned bit;
    };

    // Intel SDM, Vol. 2A, CPUID; AMD reports the same bits
    constexpr cpuid_flag Pclmul     { .leaf = 1, .subleaf = 0, .reg = ecx, .bit = 1 };
    constexpr cpuid_flag Sse41      { .leaf = 1, .subleaf = 0, .reg = ecx, .bit = 19 };
    constexpr cpuid_flag Osxsave    { .leaf = 1, .subleaf = 0, .reg = ecx, .bit = 27 };   ///< The OS enabled `xgetbv`
    constexpr cpuid_flag Avx512f    { .leaf = 7, .subleaf = 0, .reg = ebx, .bit = 16 };
    constexpr cpuid_flag Avx512vl   { .leaf = 7, .subleaf = 0, .reg = ebx, .bit = 31 };
    constexpr cpuid_flag Vpclmulqdq { .leaf = 7, .subleaf = 0, .reg = ecx, .bit = 10 };

    /**
     * @brief   Whether the CPU reports `flag`; false for a leaf above the
     *          highest one.
     */
    auto has(cpuid_flag flag) -> bool {
        static const auto max_leaf = cpuid(0, 0)[eax];
        if (flag.leaf > max_leaf) {
            return false;
        }
        return ((cpuid(flag.leaf, flag.subleaf)[flag.reg] >> flag.bit) & 1U) != 0;
    }

    // State components of XCR0 (Intel SDM, Vol. 1, 13.1)
    constexpr std::uint64_t XcrSse       = 1U << 1U;   ///< XMM registers
    constexpr std::uint64_t XcrAvx       = 1U << 2U;   ///< Upper halves of the YMM registers
    constexpr std::uint64_t XcrOpmask    = 1U << 5U;   ///< AVX-512 mask registers k0-k7
    constexpr std::uint64_t XcrZmmHi256  = 1U << 6U;   ///< Upper halves of ZMM0-ZMM15
    constexpr std::uint64_t XcrHi16Zmm   = 1U << 7U;   ///< ZMM16-ZMM31
    constexpr std::uint64_t XcrAvx512    = XcrSse | XcrAvx | XcrOpmask | XcrZmmHi256 | XcrHi16Zmm;

    /**
     * @brief   XCR0: the state components the OS saves on a context switch.
     *
     * The CPU may have registers the OS does not save, and an instruction using
     * them faults. Readable only if the OS enabled it (`Osxsave`).
     */
    auto os_saved_state() -> std::uint64_t {
#if defined(NOVA_MSVC)
        return _xgetbv(0);
#else
        // The intrinsic would need the `xsave` target for its caller
        std::uint32_t lo = 0;
        std::uint32_t hi = 0;
        __asm__ volatile("xgetbv" : "=a"(lo), "=d"(hi) : "c"(0));
        return (static_cast<std::uint64_t>(hi) << 32U) | lo;
#endif
    }

    /**
     * @brief   The flags, with the AVX-512 ones only if the OS saves the AVX-512
     *          registers.
     */
    auto detect() -> cpu_features {
        cpu_features f;
        f.sse4_1 = has(Sse41);
        f.pclmul = has(Pclmul);

        if (not has(Osxsave) || (os_saved_state() & XcrAvx512) != XcrAvx512) {
            return f;
        }

        f.avx512f    = has(Avx512f);
        f.avx512vl   = has(Avx512vl);
        f.vpclmulqdq = has(Vpclmulqdq);
        return f;
    }

#else // if defined(NOVA_X86_64)

    auto detect() -> cpu_features {
        cpu_features f;
#if defined(__ARM_FEATURE_CRC32)
        f.crc32 = true;
#endif
        return f;
    }

#endif

} // namespace

auto cpu() -> const cpu_features& {
    static const auto features = detect();
    return features;
}

} // namespace nova::detail
