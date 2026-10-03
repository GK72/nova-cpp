#include <libnova/value.hpp>

#include <libnova/test_utils.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <tuple>

TEST(Attr, ScalarRoundtrip) {
    auto v = nova::value(42);
    EXPECT_EQ(v.as<int>(), 42);
    EXPECT_EQ(v.as<std::int64_t>(), 42);

    auto d = nova::value(3.5);
    EXPECT_FLOAT_EQ(d.as<float>(), 3.5);
    EXPECT_DOUBLE_EQ(d.as<double>(), 3.5);

    auto s = nova::value("hello");
    EXPECT_EQ(s.as<std::string>(), "hello");
}

TEST(Attr, AccessThrowsOnMismatch) {
    auto v = nova::value("not-a-number");
    EXPECT_THROWN_MESSAGE(std::ignore = v.as<std::int64_t>(), "Cannot convert string to int64");
}

// Each case here only exercises names that `value_union::name_of<T>()` knows without
// reflection (its fixed table); the reflection-sourced fallback (for e.g. `int` or
// `custom_type`, which aren't variant alternatives) is covered under `AttrRefl` instead.
TEST(Attr, AsErrorNamesEveryTargetTypeKind) {
    EXPECT_THROWN_MESSAGE(std::ignore = nova::value(42).as<std::monostate>(), "Cannot convert int64 to null");
    EXPECT_THROWN_MESSAGE(std::ignore = nova::value("x").as<bool>(), "Cannot convert string to bool");
    EXPECT_THROWN_MESSAGE(std::ignore = nova::value(true).as<std::string>(), "Cannot convert bool to string");
}

TEST(Attr, MapAccess) {
    auto v = nova::value::map({
        { "host", "localhost" },
        { "port", 8080 },
    });

    EXPECT_EQ(v["host"].as<std::string>(), "localhost");
    EXPECT_EQ(v["port"].as<int>(), 8080);
    EXPECT_EQ(v.find("missing"), std::nullopt);
    EXPECT_THROWN_MESSAGE(std::ignore = v["missing"], "Missing key 'missing'");
}

TEST(Attr, SeqAccess) {
    auto v = nova::value::seq({ 1, 2, 3 });
    EXPECT_EQ(v[0].as<int>(), 1);
    EXPECT_EQ(v[2].as<int>(), 3);
    EXPECT_THROWN_MESSAGE(std::ignore = v[3], "Index 3 out of range");
}

TEST(Attr, TypeDeduction) {
    auto dict = nova::value::map({
        { "int", 1 },
        { "float", 2.2 },
        { "bool", true },
    });

    EXPECT_EQ(dict["int"].as<int>(), 1);
    EXPECT_FLOAT_EQ(dict["float"].as<float>(), 2.2f);
    EXPECT_TRUE(dict["bool"].as<bool>());
}
