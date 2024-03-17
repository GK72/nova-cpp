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
#include <expected>
#include <stdexcept>
#include <string>

namespace nova {

class assertion_error : public std::runtime_error {
public:
    assertion_error(const char* msg)
        : std::runtime_error(msg)
    {}
};

class parsing_error : public std::runtime_error {
public:
    parsing_error(const char* msg)
        : std::runtime_error(msg)
    {}
};

/**
 * @brief   Error type for `std::expected`.
 */
struct error {
    std::string message;
};

/**
 * @brief   Throwing exception with useful message in case of bad expected access.
 */
template <typename T>
class expected : public std::expected<T, error> {
public:
    using base = std::expected<T, error>;
    using base::expected;

    [[nodiscard]] auto value() {
        if (not base::has_value()) {
            throw std::runtime_error(base::error().message);
        }
        return base::value();
    }
};

using unexpected = std::unexpected<error>;

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
