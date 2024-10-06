#include <gtest/gtest.h>

#include "nova/cast.h"

#include <chrono>
#include <stdexcept>
#include <tuple>

using namespace std::chrono_literals;

namespace nova {
    using namespace alpha;
}

/**
 * @brief   An uncopyable, unmovable type.
 */
template <typename T>
class anchor {
public:
    anchor(const T& value) : m_value(value) {}
    anchor(const anchor&) = delete;
    anchor(anchor&&) = delete;
    anchor& operator=(const anchor&) = delete;
    anchor& operator=(anchor&&) = delete;

    operator T() { return m_value; }

private:
    T m_value;

};

TEST(Cast, AsCastExplicit_AnchoredType_In) {
    auto x = anchor{ 2 };
    auto y = nova::as<short>(x);
    EXPECT_EQ(y, 2);

    EXPECT_EQ(nova::as<short>(anchor{ 2 }), 2);
}

TEST(Cast, AsCastImplicit_AnchoredType_In) {
    short x = nova::as(anchor{ 2 });
    EXPECT_EQ(x, 2);
}

TEST(Cast, AsCastImplicit_AnchoredType_InOut) {
    anchor<long> x = nova::as(anchor{ 2 });
    long y = x;
    EXPECT_EQ(y, 2);
}

TEST(Cast, AsCastImplicit_Lvalue) {
    int x = 1;
    short y = nova::as(x);
    EXPECT_EQ(y, 1);
}

// TODO: find a use-case
// TEST(Cast, AsCastImplicit_Rvalue) {
    // constexpr auto func = [](short x) { return x; };

    // EXPECT_EQ(func(-2), -2);
// }

TEST(Cast, AsCast_ToNumber) {
    int x = nova::as("123");
    EXPECT_EQ(x, 123);

    // TODO(design): error handling; should `as` fail, and if yes, how?
    EXPECT_THROW(std::ignore = nova::as<int>("a123"), std::runtime_error);

    int y = nova::as("1234a");
    EXPECT_EQ(y, 1234);
}
