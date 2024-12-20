/**
 * Part of Nova C++ Library.
 *
 * Compiler magics, macros, instrinsics, etc...
 *
 * TODO(feat): `is_debugger_present()` is not implemented for Mac and
 *             always returns with false.
 */

#pragma once

#include <cstdint>
#include <fstream>
#include <string>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    #define NOVA_WIN
#elif defined(__linux__)
    #define NOVA_LINUX
#elif defined(__APPLE__)
    #define NOVA_MACOS
#endif

#if defined(_MSC_VER)
    #define NOVA_MSVC
#elif defined(__clang__)
    #define NOVA_CLANG
#elif defined(__GNUC__)
    #define NOVA_GCC
#endif

#ifdef NOVA_MSVC
    #include "intrin.h"
#else
    #ifdef __x86_64__
        #include "x86intrin.h"
    #endif
#endif

namespace nova {

/**
 * @brief   Read time-stamp counter. It measures the CPU cycles since reset.
 *
 * The instructions loads the high-order 32 bits into EDX, and the low-order
 * into EAX.
 *
 * Overflow: 2 ^ 64 cycles @3 GHz ~ 195 years
 *
 * Note: only x86_64 is supported!
 *
 * # Reference
 *
 * https://www.ccsl.carleton.ca/~jamuir/rdtscpm1.pdf
 */
[[nodiscard]] inline auto rdtsc() -> std::uint64_t {
    #ifdef __aarch64__
        return 0;
    #else
        return __rdtsc();
    #endif
}

#ifdef NOVA_LINUX
    [[nodiscard]] inline auto is_debugger_present() -> bool {
        std::ifstream inf("/proc/self/status");

        for (std::string line; std::getline(inf, line); ) {
            static constexpr auto PrefixLength = 11;
            if (line.compare(0, PrefixLength, "TracerPid:\t") == 0) {
                return line.length() > PrefixLength && line[PrefixLength] != '0';
            }
        }

        return true;
    }
#else
    [[nodiscard]] inline auto is_debugger_present() -> bool {
        return false;
    }
#endif

/**
 * @brief   Invokes undefined behaviour for optimizing impossible code branches away.
 *
 * Available in C++23.
 * - GCC 12
 * - Clang 15
 * - MSVC 19.32
 * - Apple Clang 14.0.3
 */
[[noreturn]] inline void unreachable() {
#if defined(_MSC_VER) && !defined(__clang__) // MSVC
    __assume(false);
#else // GCC, Clang
    __builtin_unreachable();
#endif
}

} // namespace nova

#if defined(NOVA_MSVC)
    #define nova_breakpoint()           if (nova::is_debugger_present()) { __debugbreak(); }
#elif defined(NOVA_CLANG)
    #define nova_breakpoint()           if (nova::is_debugger_present()) { __builtin_debugtrap(); }
#elif defined(NOVA_GCC)
    #if (defined(__i386__) || defined(__x86_64__))
        #define nova_breakpoint()       if (nova::is_debugger_present()) { __asm__ volatile("int $0x03"); }
    #elif defined(__aarch64__)
        // https://developer.arm.com/documentation/ddi0602/2023-12/Base-Instructions/BRK--Breakpoint-instruction-?lang=en
        #define nova_breakpoint()       if (nova::is_debugger_present()) { __asm__ volatile("brk #0"); }
    #endif
#else
    #define nova_breakpoint()
    // TODO(x-platform): emit a non-error warning that every major compiler likes
    #warning "Setting breakpoint from code is not supported on the target platform!"
#endif
