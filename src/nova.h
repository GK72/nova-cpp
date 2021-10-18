#include <algorithm>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <source_location>
#include <string_view>
#include <vector>

#include "string_utils.h"

namespace nova {

namespace detail {

    [[nodiscard]] auto summaryHeader() {
        return utl::joinStr("",
            "\n",
            utl::colorize(term_colors::strong::darkblue, "----==[ "),
            utl::colorize(term_colors::strong::blue, "SUMMARY"),
            utl::colorize(term_colors::strong::darkblue, " ]==----")
        );
    };

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

        // TODO: testcases should not run themselves if we would like to filter them
        //       and not run them all

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
        static constexpr auto symbol = "=";
        static constexpr auto message = "is equal to";

        template <class Expected, class Actual>
            requires std::equality_comparable_with<Expected, Actual>
        [[nodiscard]] constexpr
        bool operator()(Expected&& expected, Actual&& actual) const noexcept {
            return std::forward<Expected>(expected) == std::forward<Actual>(actual);
        }
    };

    struct lt_comp {
        static constexpr auto symbol = "<";
        static constexpr auto message = "is less than";

        template <class Expected, class Actual>
            requires std::totally_ordered_with<Expected, Actual>
        [[nodiscard]] constexpr
        bool operator()(Expected&& expected, Actual&& actual) const noexcept {
            return std::forward<Expected>(expected) < std::forward<Actual>(actual);
        }
    };

    // ---------------------------------------------------------------------------------------------

    struct TestCase {
        std::string_view name;
        bool result;

        template <class Callable>
        void operator=(Callable func) {     // NOLINT: hijacked assigment operator (hence void return type)
            utl::print(" ", "Running", name);
            auto t1 = std::chrono::system_clock::now();

            // TODO: inject time threshold here via checker?
            const auto checker = func();
            auto t2 = std::chrono::system_clock::now();

            result = checker;
            if (result) {
                utl::print("", "   ", utl::colorize(term_colors::green, utf::checkMark));
                utl::println("", "  ", utl::colorize(term_colors::strong::black, t2 - t1));
            }
            else {
                // TODO: register timer threshold and make it fail here if it exceeds it
                utl::print("", "   ", utl::colorize(term_colors::strong::red, utf::ballot));
                utl::println("", "  ", utl::colorize(term_colors::strong::black, t2 - t1));
                utl::println(checker.msg());
            }

            detail::testCom().registerTestCase(*this);
        }
    };

    inline void TestCom::printSummary() {
        const auto fails = std::ranges::count_if(m_tests, [](const TestCase& test) { return !test.result; });
        utl::println("\n",
            summaryHeader(),
            utl::indent(2,
                fails > 0
                    ? utl::joinStr(" ", fails, "tests failed out of", m_tests.size())
                    : utl::joinStr(" ", m_tests.size(), "tests successfully completed")
            )
        );
    }

    template <utl::stringable Expected, utl::stringable Actual, class Pred>
        requires std::predicate<Pred, Expected, Actual>
    class Check {
    public:
        constexpr Check(
                Expected&& expected, Actual&& actual, Pred&& p,
                const std::source_location& location = std::source_location::current()
        ) noexcept
            : m_expected(std::forward<Expected>(expected))
            , m_actual(std::forward<Actual>(actual))
            , m_pred(std::forward<Pred>(p))
            , m_loc(location)
        {}

        Check(const Check&)            = delete;
        Check(Check&&)                 = delete;
        Check& operator=(const Check&) = delete;
        Check& operator=(Check&&)      = delete;

        ~Check() = default;

        [[nodiscard]] std::string msg() const {
            return utl::joinStr("\n",
                utl::joinStr("",
                    "Expectation ", utl::colorize(term_colors::red, "failure"), " at ",
                    utl::colorize(term_colors::strong::black, m_loc)
                ),
                diffMsg()
            );
        }

        [[nodiscard]] auto diffMsg() const {
            if constexpr (!std::is_compound_v<Expected> && !std::is_compound_v<Actual>) {
                return utl::joinStr(" ",
                    utl::colorize(term_colors::blue, m_expected),
                    Pred::symbol,
                    utl::colorize(term_colors::strong::red, m_actual)
                );
            }
            else {
                return utl::joinStr("\n",
                    "The expected...",
                    utl::colorize(term_colors::blue, m_expected),
                    utl::joinStr(" ", Pred::message, "the actual..."),
                    utl::colorize(term_colors::strong::red, m_actual)
                );
            }
        }

        [[nodiscard]] operator bool() const noexcept {
            return m_pred(m_expected, m_actual);
        }

    private:
        Expected m_expected;
        Actual m_actual;
        Pred m_pred;
        std::source_location m_loc;
    };

} // namespace detail

// --------------------------------------==[ Wrappers ]==-------------------------------------------

template <class Expected, class Actual>
struct eq : public detail::Check<Expected, Actual, detail::eq_comp> {
    constexpr eq(Expected&& expected, Actual&& actual) noexcept
        : detail::Check<Expected, Actual, detail::eq_comp>(
            std::forward<Expected>(expected),
            std::forward<Actual>(actual),
            detail::eq_comp()
        )
    {}
};

template <class Expected, class Actual>
struct lt : public detail::Check<Expected, Actual, detail::lt_comp> {
    constexpr lt(Expected&& expected, Actual&& actual) noexcept
        : detail::Check<Expected, Actual, detail::lt_comp>(
            std::forward<Expected>(expected),
            std::forward<Actual>(actual),
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
