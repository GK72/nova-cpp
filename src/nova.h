#include <algorithm>
#include <concepts>
#include <cstddef>
#include <source_location>
#include <string_view>
#include <vector>

#include "string_utils.h"

namespace nova {

namespace detail {

    struct TestCase;

    /**
     * A class for global state
     */
    class TestCom {
    public:
        TestCom() = default;

        TestCom(const TestCom&)            = delete;
        TestCom(TestCom&&)                 = delete;
        TestCom& operator=(const TestCom&) = delete;
        TestCom& operator=(TestCom&&)      = delete;

        ~TestCom() {
            printSummary();
        }

        void registerTestCase(const TestCase& test) {
            m_tests.push_back(test);
        }

        void printSummary();

    private:
        std::vector<TestCase> m_tests;
    };

    /**
     * @brief   Access to the global state
     *
     * Guideline: no singleton, no global variable
     */
    TestCom& testCom() {
        static TestCom x;
        return x;
    }

    // ------------------------------==[ Comparison callables ]==-----------------------------------

    struct eq_comp {
        static constexpr auto message = "=";

        template <class Lhs, class Rhs> requires std::equality_comparable_with<Lhs, Rhs>
        [[nodiscard]] constexpr
        bool operator()(Lhs&& lhs, Rhs&& rhs) const noexcept {
            return std::forward<Lhs>(lhs) == std::forward<Rhs>(rhs);
        }
    };

    struct lt_comp {
        static constexpr auto message = "<";

        template <class Lhs, class Rhs> requires std::totally_ordered_with<Lhs, Rhs>
        [[nodiscard]] constexpr
        bool operator()(Lhs&& lhs, Rhs&& rhs) const noexcept {
            return std::forward<Lhs>(lhs) < std::forward<Rhs>(rhs);
        }
    };

    // ---------------------------------------------------------------------------------------------

    struct TestCase {
        std::string_view name;
        bool result;

        template <class Callable>
        void operator=(Callable func) {     // NOLINT: hijacked assigment operator (hence void return type)
            utl::print(" ", "Running", name);

            // Lifetime extension of the checker to print its message at the end of this block
            // and not before the status printing
            const auto checker = func();

            result = checker;
            if (result) {
                utl::println("", "   ", utl::colorize(term_colors::green, utf::checkMark));
            }
            else {
                utl::println("", "   ", utl::colorize(term_colors::red, utf::ballot));
            }

            detail::testCom().registerTestCase(*this);
        }
    };

    inline void TestCom::printSummary() {
        // TODO: name of the failed tests
        utl::println(" ",
            "Failed tests:",
            std::ranges::count_if(m_tests, [](const TestCase& test) { return !test.result; })
        );
    }

    template <class Lhs, class Rhs, class Pred>
        requires
            stringable<Lhs> &&
            stringable<Rhs> &&
            std::predicate<Pred, Lhs, Rhs>
    class Check {
    public:
        constexpr Check(
                Lhs&& lhs, Rhs&& rhs, Pred&& p,
                const std::source_location& location = std::source_location::current()
        ) noexcept
            : m_lhs(std::forward<Lhs>(lhs))
            , m_rhs(std::forward<Rhs>(rhs))
            , m_pred(std::forward<Pred>(p))
            , m_loc(location)
        {}

        Check(const Check&)            = delete;
        Check(Check&&)                 = delete;
        Check& operator=(const Check&) = delete;
        Check& operator=(Check&&)      = delete;

        ~Check() noexcept {
            // TODO: pretty formatting compound types
            if (not *this) {
                utl::println("", "Expectation failed at ", utl::colorize(term_colors::blue, m_loc));
                utl::println(" ", m_lhs, Pred::message, m_rhs);
            }
        }

        operator bool() const noexcept {
            return m_pred(m_lhs, m_rhs);
        }

    private:
        Lhs m_lhs;
        Rhs m_rhs;
        Pred m_pred;
        std::source_location m_loc;
    };

} // namespace detail

// --------------------------------------==[ Wrappers ]==-------------------------------------------

template <class Lhs, class Rhs>
struct eq : public detail::Check<Lhs, Rhs, detail::eq_comp> {
    constexpr eq(Lhs&& lhs, Rhs&& rhs) noexcept
        : detail::Check<Lhs, Rhs, detail::eq_comp>(
            std::forward<Lhs>(lhs),
            std::forward<Rhs>(rhs),
            detail::eq_comp()
        )
    {}
};

template <class Lhs, class Rhs>
struct lt : public detail::Check<Lhs, Rhs, detail::lt_comp> {
    constexpr lt(Lhs&& lhs, Rhs&& rhs) noexcept
        : detail::Check<Lhs, Rhs, detail::lt_comp>(
            std::forward<Lhs>(lhs),
            std::forward<Rhs>(rhs),
            detail::lt_comp()
        )
    {}
};

namespace literals {
    constexpr auto operator""_test(const char* name, std::size_t size) noexcept {
        return detail::TestCase{.name = { name, size } };
    }
}

} // namespace nova
