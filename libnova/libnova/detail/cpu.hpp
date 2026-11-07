/**
 * Part of Nova C++ Library.
 *
 * @brief   The CPU's instruction set extensions, for choosing an
 *          implementation at runtime.
 *
 * Code for an extension is compiled for that function only (`NOVA_TARGET`),
 * and called only when `cpu()` reports the extension, so the library still
 * runs on any CPU of the architecture without `-march`. See the SIMD Guide
 * (`doc/simd-guide.adoc#runtime-dispatch`).
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-11-07
 * @updated     2026-11-07
 * @status      experimental
 * @reviewed    read
 * SPDX-License-Identifier: BSL-1.0
 */

#pragma once

#include <libnova/intrinsics.hpp>

#if defined(__x86_64__) || defined(_M_X64)
    #define NOVA_X86_64
#elif defined(__aarch64__) || defined(_M_ARM64)
    #define NOVA_ARM64
#endif

/**
 * @brief   Enables instruction set extensions for one function, e.g.,
 *          `NOVA_TARGET("pclmul,sse4.1")`. MSVC needs no permission.
 */
#if defined(NOVA_MSVC)
    #define NOVA_TARGET(features)
#else
    #define NOVA_TARGET(features) __attribute__((target(features)))
#endif

namespace nova::detail {

/**
 * @brief   Extensions both the CPU and the OS support: an AVX-512 extension
 *          also needs the OS to save the 512-bit registers.
 */
struct cpu_features {
    // x86-64
    bool sse4_1     = false;
    bool pclmul     = false;
    bool avx512f    = false;
    bool avx512vl   = false;
    bool vpclmulqdq = false;    ///< 256- and 512-bit carry-less multiply; reported with AVX-512 only

    // ARM64
    bool crc32      = false;    ///< Only when the compiler enables it for the whole build, e.g., Apple silicon
};

/**
 * @brief   The features of the CPU the process runs on, detected on the first call.
 */
[[nodiscard]] auto cpu() -> const cpu_features&;

} // namespace nova::detail
