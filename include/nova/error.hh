/**
 * Part of Nova C++ Library.
 *
 * Error handling.
 * - One exception type for all unexpected errors. Use it when the caller cannot
 *   directly handle the error and instead it propagates the error further.
 * - Custom assert macro with auto breakpoint under debugger.
 */

#pragma once

#include "nova/intrinsics.hh"

#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include <cassert>
#include <exception>
#include <source_location>
#include <string>

#ifdef NOVA_EXPERIMENTAL_FEATURE_SET
#include <stacktrace>
#endif

namespace nova {

/**
 * @brief   Exception class with source location.
 *
 * Stacktrace can be enabled as an experimental feature (with `NOVA_EXPERIMENTAL_FEATURE_SET` macro).
 */
class exception : public std::exception {
public:
    exception(
        const std::string& msg,
        const std::source_location& location = std::source_location::current()
    #ifdef NOVA_EXPERIMENTAL_FEATURE_SET
        , const std::stacktrace& trace = std::stacktrace::current()
    #endif
    )
        : m_message(msg)
        , m_location(location)
    #ifdef NOVA_EXPERIMENTAL_FEATURE_SET
        , m_backtrace(trace)
    #endif
    {}

    template <typename ...Args>
    exception(fmt::format_string<Args...> fmt, Args&& ...args)
        : exception(fmt::format(fmt, std::forward<Args>(args)...))
    {}

    [[nodiscard]] auto what() const noexcept -> const char* override {
        return m_message.c_str();
    }

    [[nodiscard]] auto where() const noexcept {
    #ifdef NOVA_EXPERIMENTAL_FEATURE_SET
        return m_location;
    #else
        return "Source location: Not supported";
    #endif
    }

    [[nodiscard]] auto backtrace() const noexcept {
    #ifdef NOVA_EXPERIMENTAL_FEATURE_SET
        return m_backtrace;
    #else
        return "Backtrace: Not supported";
    #endif
    }

private:
    std::string m_message;
    std::source_location m_location;
    #ifdef NOVA_EXPERIMENTAL_FEATURE_SET
    std::stacktrace m_backtrace;
    #endif

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

    [[nodiscard]] auto operator<=>(const error&) const = default;
};

} // namespace nova

#ifdef NOVA_RUNTIME_ASSERTIONS
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage) | Must be a macro; `expr` needs to be converted to text
    #define nova_assert(expr)                                                   \
        if (not (expr)) {                                                       \
            nova_breakpoint();                                                  \
            throw nova::exception("Assertion failed: " #expr);                  \
        }
#else
    // NOLINTNEXTLINE(cppcoreguidelines-macro-usage) | Must be a macro; `expr` needs to be converted to text
    #define nova_assert(expr) \
        assert(expr)
#endif

/**
 * @brief   A convenience macro for throwing an exception.
 *
 * Useful for quick prototyping.
 */
#define THROWUP throw nova::exception("ERROAR");

template <>
class fmt::formatter<std::source_location> {
public:
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    /**
    * @example  nova/unit-tests/error.cc:7, from function `void func()`
    */
    template <typename FmtContext>
    constexpr auto format(const std::source_location& loc, FmtContext& ctx) const {
        return fmt::format_to(
            ctx.out(),
            "{}:{}, from function `{}`",
            loc.file_name(),
            loc.line(),
            loc.function_name()
        );
    }
};

#ifdef NOVA_EXPERIMENTAL_FEATURE_SET
template <>
class fmt::formatter<std::stacktrace_entry> {
public:
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template <typename FmtContext>
    constexpr auto format(const std::stacktrace_entry& entry, FmtContext& ctx) const {
        return fmt::format_to(
            ctx.out(),
            "{}:{} : {}",
            entry.source_file(),
            entry.source_line(),
            entry.description()
        );
    }
};

template <>
class fmt::formatter<std::stacktrace> {
public:
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    /**
    * @example  nova/unit-tests/error.cc:8 : func()
    *           nova/unit-tests/error.cc:13 : Error_Exception_Test::TestBody()
    *
    */
    template <typename FmtContext>
    constexpr auto format(const std::stacktrace& trace, FmtContext& ctx) const {
        return std::format_to(
            ctx.out(),
            "{}",
            fmt::format("{}", fmt::join(trace, "\n"))
        );
    }
};
#endif // NOVA_EXPERIMENTAL_FEATURE_SET

template <>
class fmt::formatter<nova::error> {
public:
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template <typename FmtContext>
    auto format(nova::error err, FmtContext& ctx) const {
        return fmt::format_to(ctx.out(), "{}", err.message);
    }
};
