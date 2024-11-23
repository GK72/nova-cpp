/**
 * Part of Nova C++ Library.
 *
 * Error handling.
 * - Exception types
 * - Custom assert macro with auto breakpoint under debugger
 */

#pragma once

#include "nova/expected.hh"
#include "nova/intrinsics.hh"

#include <cassert>
#include <stdexcept>
#include <string>

namespace nova {

class assertion_error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class parsing_error : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class not_implemented : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

/**
 * @brief   Error type for `expected`.
 */
struct error {
    // P0960R3 is not implemented in Apple Clang
    constexpr error(const std::string& msg)
        : message(msg)
    {}

    std::string message;
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
