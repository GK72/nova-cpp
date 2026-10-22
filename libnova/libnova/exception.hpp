/**
 * Part of Nova C++ Library.
 *
 * @brief   Exception with location and stacktrace.
 *
 * - One exception type for all unexpected errors. Use it when error cannot be
 *   handled in a graceful manner.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2024-10-27
 * @updated     2026-10-22
 * @status      Experimental API (unstable)
 */

#pragma once

#include <libnova/intrinsics.hpp>

#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include <cassert>
#include <exception>
#include <source_location>
#include <string>
#include <type_traits>
#include <utility>

#ifdef NOVA_EXPERIMENTAL_FEATURE_SET
#include <stacktrace>
#endif

namespace nova {

namespace detail {

    /**
     * @brief   A format string that records where it is written, i.e., the throw.
     *
     * A default argument is evaluated where the call is written, so the location must be taken
     * here, not by the constructor the exception delegates to.
     */
    template <typename ...Args>
    struct located_format_string {
        /**
         * @brief   Capture the location of the call.
         *
         * `std::string` is excluded: accepting it makes `exception(std::string, location)`,
         * the delegation below included, ambiguous.
         */
        template <typename S>
            requires std::is_convertible_v<const S&, fmt::string_view>
                and (not std::is_same_v<S, std::string>)
        consteval located_format_string(
            const S& str,
            const std::source_location& loc = std::source_location::current()
        )
            : fmt(str)
            , location(loc)
        {}

        fmt::format_string<Args...> fmt;
        std::source_location location;
    };

} // namespace detail

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

    /**
     * @brief   Format string overload which records the location of the caller.
     */
    template <typename ...Args>
    exception(detail::located_format_string<std::type_identity_t<Args>...> fmt, Args&& ...args)
        : exception(fmt::format(fmt.fmt, std::forward<Args>(args)...), fmt.location)
    {}

    [[nodiscard]] auto what() const noexcept -> const char* override {
        return m_message.c_str();
    }

    [[nodiscard]] auto where() const noexcept -> const std::source_location& {
        return m_location;
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

} // namespace nova

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
