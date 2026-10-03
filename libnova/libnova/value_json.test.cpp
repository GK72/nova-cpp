#include <libnova/value_json.hpp>

#include <libnova/test_utils.hpp>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

TEST(ValueJson, ParseScalars) {
    EXPECT_EQ(nova::parse_json("null").as<std::monostate>(), std::monostate{});
    EXPECT_TRUE(nova::parse_json("true").as<bool>());
    EXPECT_EQ(nova::parse_json("42").as<std::int64_t>(), 42);
    EXPECT_DOUBLE_EQ(nova::parse_json("3.5").as<double>(), 3.5);
    EXPECT_EQ(nova::parse_json(R"("hello")").as<std::string>(), "hello");
}

TEST(ValueJson, ParseArray) {
    auto v = nova::parse_json("[1, 2, 3]");
    EXPECT_EQ(v[0].as<int>(), 1);
    EXPECT_EQ(v[1].as<int>(), 2);
    EXPECT_EQ(v[2].as<int>(), 3);
}

TEST(ValueJson, ParseObject) {
    auto v = nova::parse_json(R"({"host": "localhost", "port": 8080})");
    EXPECT_EQ(v["host"].as<std::string>(), "localhost");
    EXPECT_EQ(v["port"].as<int>(), 8080);
}

TEST(ValueJson, ParseNested) {
    auto v = nova::parse_json(R"({
        "name": "svc",
        "ports": [80, 443],
        "labels": {"env": "prod"}
    })");

    EXPECT_EQ(v["name"].as<std::string>(), "svc");
    EXPECT_EQ(v["ports"][0].as<int>(), 80);
    EXPECT_EQ(v["ports"][1].as<int>(), 443);
    EXPECT_EQ(v["labels"]["env"].as<std::string>(), "prod");
}

TEST(ValueJson, ParseInvalidThrows) {
    EXPECT_THROWN_MESSAGE(nova::parse_json("not json"), "Parsing error");
}

TEST(ValueJson, DumpRoundtrip) {
    auto v = nova::value::map({
        { "name", "svc" },
        { "ports", nova::value::seq({ 80, 443 }) },
        { "ok", true },
        { "nothing", nova::value() },
    });

    auto text = nova::dump_json(v);
    auto roundtripped = nova::parse_json(text);

    EXPECT_EQ(roundtripped["name"].as<std::string>(), "svc");
    EXPECT_EQ(roundtripped["ports"][0].as<int>(), 80);
    EXPECT_EQ(roundtripped["ports"][1].as<int>(), 443);
    EXPECT_TRUE(roundtripped["ok"].as<bool>());
    EXPECT_TRUE(roundtripped["nothing"].is<std::monostate>());
}

TEST(ValueJson, DumpIndent) {
    auto v = nova::value::map({ { "x", 1 } });
    EXPECT_EQ(nova::dump_json(v), R"({"x":1})");
    EXPECT_THAT(nova::dump_json(v, 2), testing::HasSubstr("\n"));
}
