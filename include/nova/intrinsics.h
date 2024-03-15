/**
 * Part of Nova C++ Library.
 *
 * Compiler magics, macros, instrinsics, etc...
 */

#pragma once

#include <fstream>
#include <string>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
    #define NOVA_WIN
#elif defined(__linux__)
    #define NOVA_LINUX
#elif defined(__APPLE__)
    #define NOVA_MACOS
#endif

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
#warning "`is_debugger_present()` is not supported on the target platform!"
#endif

#if defined(_MSC_VER)
    #define nova_breakpoint()           __debugbreak()
#elif defined(__clang__)
    #define nova_breakpoint()           if (is_debugger_present()) { __builtin_debugtrap(); }
#elif defined(__GNUC__)
    #if (defined(__i386__) || defined(__x86_64__))
        #define nova_breakpoint()       if (is_debugger_present()) { __asm__ volatile("int $0x03"); }
    #elif defined(__aarch64__)
        // https://developer.arm.com/documentation/ddi0602/2023-12/Base-Instructions/BRK--Breakpoint-instruction-?lang=en
        #define nova_breakpoint()       if (is_debugger_present()) { __asm__ volatile("brk #0"); }
    #endif
#else
    #define nova_breakpoint()
    #warning "Setting breakpoint from code is not supported on the target platform!"
#endif
