/**
 * Part of Nova C++ Library.
 *
 * Error handling.
 * - Exception types
 * - Custom assert macro with auto breakpoint under debugger
 */

#pragma once

#include "nova/intrinsics.h"

#include <cassert>
#include <stdexcept>

namespace nova {

class assertion_error : public std::runtime_error {
public:
    assertion_error(const char* msg)
        : std::runtime_error(msg)
    {}
};

} // namespace nova

#ifdef NOVA_RUNTIME_ASSERTIONS
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage) | Must be a macro; `expr` needs to be converted to text
    #define nova_assert(expr)                                                   \
        if (not static_cast<bool>(expr)) {                                      \
            nova_breakpoint();                                                  \
            throw nova::assertion_error("Assertion failed: " #expr);            \
        }
#else
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage) | Must be a macro; `expr` needs to be converted to text
    #define nova_assert(expr) \
        assert(expr)
#endif
