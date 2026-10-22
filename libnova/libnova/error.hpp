/**
 * Part of Nova C++ Library.
 *
 * @brief   Error type for `expected`.
 *
 * - Built on `std::error_code`: what went wrong, and its origin (category).
 * - A trait that turns an enum into an error origin.
 * - `wrap_error` to return an error from a function returning `expected`.
 * - Custom assert macro with auto breakpoint under debugger.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2024-10-27
 * @updated     2026-10-22
 * @status      Experimental API (unstable)
 */

#pragma once

#include <libnova/exception.hpp>
#include <libnova/expected.hpp>

#include <fmt/format.h>

#include <concepts>
#include <cstddef>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>

namespace nova {

/**
 * @brief   Turns an enum into an error origin; specialize it next to the enum.
 *
 * - `name`: the category.
 * - `messages`: one per value, from the value one (zero means success).
 * - `conditions` (optional): the `std::errc` per value, for checks that do not
 *   care about the origin.
 *
 * `nova::error` and `wrap_error` take such an enum as is. For a plain
 * `std::error_code` conversion, use `using nova::make_error_code;` in the
 * enum's namespace.
 *
 * @example
 *
 * ```
 * template <>
 * struct nova::error_enum<networking_error> {
 *     static constexpr auto name = "net";
 *     static constexpr auto messages = std::to_array<std::string_view>({ "timed out", "connection refused" });
 * };
 * ```
 */
template <typename E>
struct error_enum {};

template <typename E>
concept error_enum_c = std::is_enum_v<E> and requires {
    { error_enum<E>::name } -> std::convertible_to<const char*>;
    { error_enum<E>::messages[std::size_t{}] } -> std::convertible_to<std::string_view>;
};

namespace detail {

    /**
     * @brief   The `std::error_category` of an `error_enum`, one per enum.
     *
     * A boilerplate helper for hooking custom error types into
     * `std::error_category`.
     */
    template <error_enum_c E>
    class enum_error_category : public std::error_category {
    public:
        [[nodiscard]] static auto instance() -> const enum_error_category& {
            static const auto category = enum_error_category{};
            return category;
        }

        [[nodiscard]] auto name() const noexcept -> const char* override {
            return error_enum<E>::name;
        }

        [[nodiscard]] auto message(int value) const -> std::string override {
            if (not in_range(value, error_enum<E>::messages)) {
                return fmt::format("unknown {} error {}", name(), value);
            }
            return std::string(error_enum<E>::messages[static_cast<std::size_t>(value - 1)]);
        }

        [[nodiscard]] auto default_error_condition(int value) const noexcept -> std::error_condition override {
            if constexpr (requires { error_enum<E>::conditions; }) {
                if (in_range(value, error_enum<E>::conditions)) {
                    return error_enum<E>::conditions[static_cast<std::size_t>(value - 1)];
                }
            }
            return { value, *this };
        }

    private:
        [[nodiscard]] static constexpr auto in_range(int value, const auto& table) -> bool {
            return value >= 1 and static_cast<std::size_t>(value) <= std::size(table);
        }
    };

} // namespace detail

template <error_enum_c E>
[[nodiscard]] auto make_error_code(E value) -> std::error_code {
    return { static_cast<int>(value), detail::enum_error_category<E>::instance() };
}

namespace detail::adl_only {

    void make_error_code() = delete;        // Hides `nova::make_error_code` from ordinary lookup

    /**
     * @brief   `std::error_code` finds `make_error_code` only by ADL.
     */
    template <typename E>
    concept has_make_error_code = requires (E value) { make_error_code(value); };

} // namespace detail::adl_only

} // namespace nova

/**
 * @brief   Implicit `std::error_code` conversion of an `error_enum`, if its
 *          namespace has `make_error_code`.
 */
template <typename E>
    requires nova::error_enum_c<E> and nova::detail::adl_only::has_make_error_code<E>
struct std::is_error_code_enum<E> : std::true_type {};

namespace nova {

/**
 * @brief   Error type for `expected`: what went wrong and what it was about.
 *
 * The code says what went wrong. Its category is the origin (`system`, `parse`,
 * `env`, ...), one per module that produces errors; any library can add its
 * own with a `std::error_category`. To react to a kind of failure regardless
 * of the origin, compare the code with a `std::errc` condition. The detail
 * names the subject (a path, a value).
 */
struct error {
    std::error_code code;
    std::string detail = {};

    [[nodiscard]] auto operator<=>(const error&) const = default;
};

/**
 * @brief   The error to return from a function returning `expected<T, error>`.
 *
 * Created from `std::error_code`; `std::errc` needs `std::make_error_code()`.
 *
 * @example `return nova::wrap_error(std::make_error_code(std::errc::is_a_directory), path.string());`
 */
[[nodiscard]] inline
auto wrap_error(std::error_code code, std::string detail = {}) -> unexpected<error> {
    return {
        error{
            .code = code,
            .detail = std::move(detail)
        }
    };
}

/**
 * @brief   The error of an `error_enum` value, even without `std::error_code` conversion.
 */
template <error_enum_c E>
[[nodiscard]] auto wrap_error(E value, std::string detail = {}) -> unexpected<error> {
    return wrap_error(nova::make_error_code(value), std::move(detail));
}

} // namespace nova

template <>
class fmt::formatter<nova::error> {
public:
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    /**
     * @format   [category] message (detail)
     * @example  `[generic] No such file or directory (/etc/missing)`; the detail only if set.
     */
    template <typename FmtContext>
    auto format(const nova::error& err, FmtContext& ctx) const {
        auto out = fmt::format_to(ctx.out(), "[{}] {}", err.code.category().name(), err.code.message());
        if (not err.detail.empty()) {
            out = fmt::format_to(out, " ({})", err.detail);
        }
        return out;
    }
};

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
