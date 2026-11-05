#include <libnova/refl.hpp>

#ifdef NOVA_EXPERIMENTAL_FEATURE_SET

#include <libnova/test_utils.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <string_view>

using namespace std::string_view_literals;

TEST(Refl, HasAnnotationOf) {
    struct type_annotation {};
    struct value_annotation { int n; };

    struct [[=type_annotation{}]] test {
        [[=value_annotation{ 42 }]] int x;
        int y;
    };

    [[=type_annotation{}]] auto v = test{ };

    static_assert(nova::refl::has_annotation_of<type_annotation>(^^test));
    static_assert(nova::refl::has_annotation_of<type_annotation>(^^v));
    static_assert(nova::refl::has_annotation_of<type_annotation>(std::meta::type_of(^^v)));

    static_assert(
        nova::refl::has_annotation_of<value_annotation>(
            std::meta::nonstatic_data_members_of(
                ^^test,
                std::meta::access_context::current()
            )[0]
        )
    );

    static_assert(
        not nova::refl::has_annotation_of<value_annotation>(
            std::meta::nonstatic_data_members_of(
                ^^test,
                std::meta::access_context::current()
            )[1]
        )
    );
}

TEST(Refl, ExtractAnnotation) {
    struct value_annotation { int n; };

    struct test {
        [[=value_annotation{ 42 }]] int x;
    };

    constexpr auto mem = std::meta::nonstatic_data_members_of(^^test, std::meta::access_context::current())[0];
    static_assert(nova::refl::extract_annotation<value_annotation>(mem).n == 42);
}

TEST(Refl, EnumToString) {
    enum class enum_1 {
        enum_a,
        enum_b
    };

    constexpr auto x = enum_1::enum_a;
    static_assert(nova::refl::enum_to_string(x) == "enum_a");
}

TEST(Refl, StringToEnum) {
    enum class enum_2 {
        enum_a,
        enum_b
    };

    constexpr enum_2 x = nova::refl::string_to_enum<enum_2>("enum_b");
    static_assert(x == enum_2::enum_b);
    EXPECT_THROWN_MESSAGE(nova::refl::string_to_enum<enum_2>("enum_c"), "Invalid enumeration: enum_c");
}

TEST(Refl, Formatter) {
    struct [[=nova::ann::fmt_debug]] point2d {
        int x;
        int y;
    };

    constexpr auto p = point2d{ .x = 6, .y = 9 };
    EXPECT_EQ(fmt::format("{}", p), "point2d{ .x=6, .y=9 }");
}

#endif // NOVA_EXPERIMENTAL_FEATURE_SET

