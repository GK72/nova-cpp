#pragma once

#include <cassert>
#include <stdexcept>

namespace nova {

class assertion_error : public std::runtime_error {
public:
    assertion_error(const char* msg)
        : std::runtime_error(msg)
    {}
};

#ifdef NOVA_RUNTIME_ASSERTIONS
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage) | Must be a macro; `expr` needs to be converted to text
#define nova_assert(expr) \
    if (not static_cast<bool>(expr)) { \
        throw nova::assertion_error("Assertion failed: " #expr); \
    }

#else
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage) | Must be a macro; `expr` needs to be converted to text
#define nova_assert(expr) \
    assert(expr)
#endif

} // namespace nova
