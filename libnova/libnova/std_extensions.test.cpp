#include <libnova/std_extensions.hpp>

#include <gtest/gtest.h>

#include <type_traits>

TEST(StdExt, ToUnderlying) {
    enum class EnumShort : short { e };
    const auto e = EnumShort::e;
    static_assert(std::is_same_v<short, decltype(nova::to_underlying(e))>);
}
