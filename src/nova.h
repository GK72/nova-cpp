#include <cstddef>
#include <source_location>
#include <string_view>

#include "stringUtils.h"

namespace nova {

struct Test {
    std::string_view name;

    template <class Callable>
    void operator=(Callable func) {     // NOLINT: hijacked operator (void)
        utl::println(" ", "Running", name);
        func();
    }
};

namespace literals {
    constexpr auto operator""_test(const char* name, std::size_t size) {
        return Test{.name = { name, size } };
    }
}

template <class Lhs, class Rhs>
class EQ {
public:
    constexpr EQ(Lhs&& lhs, Rhs&& rhs, const std::source_location& location = std::source_location::current())
        : m_lhs(std::forward<Lhs>(lhs))
        , m_rhs(std::forward<Rhs>(rhs))
        , m_loc(location)
    {}

    EQ(const EQ&)            = delete;
    EQ(EQ&&)                 = delete;
    EQ& operator=(const EQ&) = delete;
    EQ& operator=(EQ&&)      = delete;

    ~EQ() noexcept {
        if (not *this) {
            utl::println(" ", "Expectation failed at", m_loc);
            utl::println(" == ", m_lhs, m_rhs);
        }
    }

    operator bool() const {
        return m_lhs == m_rhs;
    }

private:
    Lhs m_lhs;
    Rhs m_rhs;
    std::source_location m_loc;

};

template <class Lhs, class Rhs> EQ(Lhs, Rhs) -> EQ<Lhs, Rhs>;

} // namespace nova
