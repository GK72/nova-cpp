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

#include <fmt/format.h>

#include <cassert>
#include <format>
#include <source_location>
#include <stdexcept>
#include <string>

#ifdef NOVA_EXPERIMENTAL_FEATURE_SET
#include <stacktrace>
#endif

namespace nova {

namespace detail {

    class exception_base : public std::runtime_error {
    public:
        exception_base(
            const std::string& msg,
            const std::source_location& location
        #ifdef NOVA_EXPERIMENTAL_FEATURE_SET
            , const std::stacktrace& trace = std::stacktrace::current()
        #endif
        )
            : std::runtime_error(msg)
            , m_location(location)
        #ifdef NOVA_EXPERIMENTAL_FEATURE_SET
            , m_backtrace(trace)
        #endif
        {}

        [[nodiscard]] auto where() const noexcept -> const std::source_location& { return m_location; }
        #ifdef NOVA_EXPERIMENTAL_FEATURE_SET
        [[nodiscard]] auto backtrace() const noexcept { return m_backtrace; }
        #endif

    protected:
        std::source_location m_location;
        #ifdef NOVA_EXPERIMENTAL_FEATURE_SET
        std::stacktrace m_backtrace;
        #endif

    };

} // namespace detail

template <typename T>
class exception : public detail::exception_base {
public:
    exception(
        const std::string& msg,
        const T& data,
        const std::source_location& location = std::source_location::current()
        #ifdef NOVA_EXPERIMENTAL_FEATURE_SET
        , const std::stacktrace& trace = std::stacktrace::current()
        #endif
    )
        : exception_base(
            msg,
            location
            #ifdef NOVA_EXPERIMENTAL_FEATURE_SET
            , trace
            #endif
        )
        , m_data(data)
    {}

    [[nodiscard]] auto data() const noexcept -> const T& { return m_data; }

private:
    T m_data;
};

template <>
class exception<void> : public detail::exception_base {
public:
    exception(
        const std::string& msg,
        const std::source_location& location = std::source_location::current()
        #ifdef NOVA_EXPERIMENTAL_FEATURE_SET
        , const std::stacktrace& trace = std::stacktrace::current()
        #endif
    )
        : exception_base(
            msg,
            location
            #ifdef NOVA_EXPERIMENTAL_FEATURE_SET
            , trace
            #endif
        )
    {}
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
        if (not static_cast<bool>(expr)) {                                      \
            nova_breakpoint();                                                  \
            throw nova::exception<void>("Assertion failed: " #expr);            \
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
#define THROWUP throw nova::exception<void>("ERROAR");

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
        return std::format_to(
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
        return std::format_to(
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
