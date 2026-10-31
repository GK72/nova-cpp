#include <libnova/dyn_value.hpp>

#include <libnova/test_utils.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

using ::testing::HasSubstr;

namespace {

using enum nova::dyn_value::format_e;

[[nodiscard]] auto parse(std::string_view text, nova::dyn_value::format_e format) -> nova::dyn_value {
    return nova::dyn_value::parse(text, format).value();
}

/**
 * @brief   `doc` written in `format` and read back.
 */
[[nodiscard]] auto round_trip(const nova::dyn_value& doc, nova::dyn_value::format_e format) -> nova::dyn_value {
    return parse(doc.dump(format).value(), format);
}

[[nodiscard]] auto error_code(nova::dyn_value::error_e value) -> std::error_code {
    return nova::make_error_code(value);
}

/**
 * @brief   Everything every format can hold.
 */
[[nodiscard]] auto portable() -> nova::dyn_value {
    return nova::dyn_value::map({
        { "name", "svc" },
        { "count", -3 },
        { "ratio", 0.1F },
        { "whole", 2.0 },
        { "enabled", true },
        { "ports", nova::dyn_value::seq({ 80, 443 }) },
        { "empty", nova::dyn_value::seq({ }) },
        { "a b", "say \"hi\"\n\ttabbed" },
        { "labels", nova::dyn_value::map({ { "env", "prod" }, { "dotted.key", "x" } }) },
        { "nested", nova::dyn_value::map({ { "inner", nova::dyn_value::map({ { "n", 1 } }) } }) },
    });
}

} // namespace

// --- YAML ---

TEST(DynValueYaml, Null) {
    EXPECT_TRUE(parse("null", yaml).is_null());
    EXPECT_TRUE(parse("~", yaml).is_null());
    EXPECT_FALSE(parse("key:", yaml).find<int>("key").has_value());
    EXPECT_EQ(parse("key:", yaml), nova::dyn_value::map({ { "key", nullptr } }));
}

TEST(DynValueYaml, Bool) {
    EXPECT_TRUE(parse("true", yaml).lookup<bool>());
    EXPECT_TRUE(parse("True", yaml).lookup<bool>());
    EXPECT_FALSE(parse("FALSE", yaml).lookup<bool>());
}

TEST(DynValueYaml, Yaml11BoolsAreStrings) {
    EXPECT_EQ(parse("yes", yaml).lookup<std::string>(), "yes");
    EXPECT_EQ(parse("on", yaml).lookup<std::string>(), "on");
    EXPECT_EQ(parse("ON", yaml).lookup<std::string>(), "ON");
}

TEST(DynValueYaml, Int) {
    EXPECT_EQ(parse("42", yaml).lookup<std::int64_t>(), 42);
    EXPECT_EQ(parse("-7", yaml).lookup<std::int64_t>(), -7);
    EXPECT_EQ(parse("+7", yaml).lookup<std::int64_t>(), 7);
    EXPECT_EQ(parse("0x1F", yaml).lookup<std::int64_t>(), 31);
    EXPECT_EQ(parse("0o17", yaml).lookup<std::int64_t>(), 15);
    EXPECT_EQ(parse("-9223372036854775808", yaml).lookup<std::int64_t>(), std::numeric_limits<std::int64_t>::min());
}

TEST(DynValueYaml, OutOfRangeIntIsFloat) {
    EXPECT_EQ(parse("9223372036854775808", yaml).type_name(), "float");
}

TEST(DynValueYaml, Float) {
    EXPECT_DOUBLE_EQ(parse("3.5", yaml).lookup<double>(), 3.5);
    EXPECT_DOUBLE_EQ(parse(".5", yaml).lookup<double>(), 0.5);
    EXPECT_DOUBLE_EQ(parse("+1e3", yaml).lookup<double>(), 1000.0);
    EXPECT_EQ(parse("-.inf", yaml).lookup<double>(), -std::numeric_limits<double>::infinity());
    EXPECT_TRUE(std::isnan(parse(".nan", yaml).lookup<double>()));
}

TEST(DynValueYaml, NonCoreNumbersAreStrings) {
    EXPECT_EQ(parse("inf", yaml).lookup<std::string>(), "inf");
    EXPECT_EQ(parse("nan", yaml).lookup<std::string>(), "nan");
    EXPECT_EQ(parse("1.2.3", yaml).lookup<std::string>(), "1.2.3");
    EXPECT_EQ(parse("0x", yaml).lookup<std::string>(), "0x");
    EXPECT_EQ(parse("12abc", yaml).lookup<std::string>(), "12abc");
}

TEST(DynValueYaml, QuotedScalarsAreStrings) {
    EXPECT_EQ(parse(R"("true")", yaml).lookup<std::string>(), "true");
    EXPECT_EQ(parse("'42'", yaml).lookup<std::string>(), "42");
    EXPECT_EQ(parse(R"("")", yaml).lookup<std::string>(), "");
}

TEST(DynValueYaml, NonScalarKeyIsUnsupported) {
    const auto doc = nova::dyn_value::parse("? [a, b]\n: value", yaml);
    ASSERT_FALSE(doc);
    EXPECT_EQ(doc.error().code, error_code(nova::dyn_value::error_e::unsupported));
    EXPECT_THAT(doc.error().detail, HasSubstr("line 1"));
}

TEST(DynValueYaml, SyntaxError) {
    const auto doc = nova::dyn_value::parse("key: [unclosed", yaml);
    ASSERT_FALSE(doc);
    EXPECT_EQ(doc.error().code, error_code(nova::dyn_value::error_e::syntax));
}

TEST(DynValueYaml, StringsThatLookLikeOtherTypesAreQuoted) {
    const auto doc = nova::dyn_value::seq({ "true", "42", "1.10", "~", "", "null", ".inf", "plain" });
    const auto text = doc.dump(yaml).value();

    EXPECT_THAT(text, HasSubstr(R"("1.10")"));
    EXPECT_THAT(text, HasSubstr("plain"));
    EXPECT_THAT(text, ::testing::Not(HasSubstr(R"("plain")")));
    EXPECT_EQ(parse(text, yaml), doc);
}

TEST(DynValueYaml, FloatsStayFloats) {
    const auto doc = nova::dyn_value::seq({ 2.0, -std::numeric_limits<double>::infinity(), 1e300 });
    EXPECT_EQ(round_trip(doc, yaml), doc);
    EXPECT_TRUE(std::isnan(round_trip(nova::dyn_value(std::nan("")), yaml).lookup<double>()));
}

TEST(DynValueYaml, NullRoundTrip) {
    const auto doc = nova::dyn_value::map({ { "gone", nullptr }, { "kept", 1 } });
    EXPECT_EQ(round_trip(doc, yaml), doc);
}

// --- JSON ---

TEST(DynValueJson, Scalars) {
    EXPECT_TRUE(parse("null", json).is_null());
    EXPECT_TRUE(parse("true", json).lookup<bool>());
    EXPECT_EQ(parse("-42", json).lookup<std::int64_t>(), -42);
    EXPECT_DOUBLE_EQ(parse("2.5", json).lookup<double>(), 2.5);
    EXPECT_EQ(parse(R"("text")", json).lookup<std::string>(), "text");
}

TEST(DynValueJson, BeyondInt64IsFloat) {
    EXPECT_EQ(parse("18446744073709551615", json).type_name(), "float");
    EXPECT_EQ(parse("9223372036854775807", json).lookup<std::int64_t>(), std::numeric_limits<std::int64_t>::max());
}

TEST(DynValueJson, SyntaxError) {
    const auto doc = nova::dyn_value::parse("{ broken", json);
    ASSERT_FALSE(doc);
    EXPECT_EQ(doc.error().code, error_code(nova::dyn_value::error_e::syntax));
}

TEST(DynValueJson, Indented) {
    const auto doc = nova::dyn_value::map({ { "a", nova::dyn_value::seq({ 1 }) } });
    EXPECT_EQ(doc.dump(json).value(), "{\n    \"a\": [\n        1\n    ]\n}\n");
}

TEST(DynValueJson, NonFiniteIsUnsupported) {
    const auto doc = nova::dyn_value::map({ { "list", nova::dyn_value::seq({ 1.0, std::numeric_limits<double>::infinity() }) } });
    const auto text = doc.dump(json);
    ASSERT_FALSE(text);
    EXPECT_EQ(text.error().code, error_code(nova::dyn_value::error_e::unsupported));
    EXPECT_THAT(text.error().detail, HasSubstr("list.1"));
}

TEST(DynValueJson, NullRoundTrip) {
    const auto doc = nova::dyn_value::map({ { "gone", nullptr }, { "kept", 1 } });
    EXPECT_EQ(round_trip(doc, json), doc);
}

// --- TOML ---

TEST(DynValueToml, TablesArraysAndScalars) {
    const auto doc = parse(R"(
        name = "space"
        scale = 1.5
        count = 3
        vsync = true
        sizes = [ 1, 2 ]

        [input]
        deadzone = 0.1
    )", toml);

    EXPECT_EQ(doc.lookup<std::string>("name"), "space");
    EXPECT_DOUBLE_EQ(doc.lookup<double>("scale"), 1.5);
    EXPECT_EQ(doc.lookup<int>("count"), 3);
    EXPECT_TRUE(doc.lookup<bool>("vsync"));
    EXPECT_EQ(doc.lookup<int>("sizes.1"), 2);
    EXPECT_FLOAT_EQ(doc.lookup<float>("input.deadzone"), 0.1F);
}

TEST(DynValueToml, SyntaxError) {
    const auto doc = nova::dyn_value::parse("a = \n[b", toml);
    ASSERT_FALSE(doc);
    EXPECT_EQ(doc.error().code, error_code(nova::dyn_value::error_e::syntax));
    EXPECT_THAT(doc.error().detail, HasSubstr("1:"));
}

TEST(DynValueToml, DatesAreUnsupported) {
    const auto doc = nova::dyn_value::parse("[when]\nstart = 2026-10-23", toml);
    ASSERT_FALSE(doc);
    EXPECT_EQ(doc.error().code, error_code(nova::dyn_value::error_e::unsupported));
    EXPECT_EQ(doc.error().detail, "when.start");
}

TEST(DynValueToml, Sections) {
    const auto text = portable().dump(toml).value();
    EXPECT_THAT(text, HasSubstr("ratio = 0.1\n"));
    EXPECT_THAT(text, HasSubstr("whole = 2.0\n"));
    EXPECT_THAT(text, HasSubstr(R"("a b" = "say \"hi\"\n\ttabbed")"));
    EXPECT_THAT(text, HasSubstr("[nested.inner]\nn = 1\n"));
}

TEST(DynValueToml, Unsupported) {
    const auto scalar = nova::dyn_value(1).dump(toml);
    ASSERT_FALSE(scalar);
    EXPECT_EQ(scalar.error().code, error_code(nova::dyn_value::error_e::unsupported));

    const auto null = nova::dyn_value::map({ { "a", nova::dyn_value::map({ { "b", nullptr } }) } }).dump(toml);
    ASSERT_FALSE(null);
    EXPECT_EQ(null.error().detail, "a.b");
}

// --- Every format ---

TEST(DynValueFormats, RoundTrip) {
    for (const auto format : { yaml, json, toml }) {
        EXPECT_EQ(round_trip(portable(), format), portable()) << portable().dump(format).value();
    }
}

TEST(DynValueFormats, EndWithNewLine) {
    for (const auto format : { yaml, json, toml }) {
        EXPECT_TRUE(portable().dump(format).value().ends_with('\n'));
    }
}

TEST(DynValueFormats, FloatKeepsItsShortestForm) {
    EXPECT_EQ(nova::dyn_value(0.1F).lookup<double>(), 0.1);
    EXPECT_EQ(nova::dyn_value(1.5F).lookup<double>(), 1.5);
}
