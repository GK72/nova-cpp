#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "nova/error.h"
#include "nova/json.h"
#include "nova/io.h"

#include <sstream>
#include <tuple>
#include <vector>

constexpr auto input = R"(
{
    "key": 1,
    "sub": {
        "int": 9,
        "float": 9.9,
        "boolean": true,
        "string": "bla"
    },
    "list": [ 1, 2, 3 ],
    "records": [
        {
            "name": "a",
            "value": 1
        },
        {
            "name": "b",
            "value": 2
        }
    ]
}

)";

TEST(Json, DomPath) {
    EXPECT_EQ(nova::dom_path("").rfc6901(), "");
    EXPECT_EQ(nova::dom_path("key").rfc6901(), "/key");
    EXPECT_EQ(nova::dom_path("key.sub").rfc6901(), "/key/sub");
    EXPECT_EQ(nova::dom_path("key.record.0").rfc6901(), "/key/record/0");
}

TEST(Json, EmptyInput) {
    EXPECT_THAT(
        []() { std::ignore = nova::json(""); },
        testing::ThrowsMessage<nova::exception<void>>(
            testing::HasSubstr("parse error at line 1, column 1: attempting to parse an empty input;")
        )
    );
}

TEST(Json, InvalidInput) {
    EXPECT_THAT(
        []() { std::ignore = nova::json(R"("key": 1)"); },
        testing::ThrowsMessage<nova::exception<void>>(
            testing::HasSubstr("parse error at line 1, column 6: syntax error while parsing value - unexpected ':';")
        )
    );
}

TEST(Json, ConstructFromJsonObject) {
    const auto doc = nova::json(input);
    const auto inner = doc.at("sub");
    EXPECT_EQ(inner.lookup<int>("int"), 9);
}

TEST(Json, Contains) {
    const auto json = nova::json(input);
    EXPECT_TRUE(json.contains("key"));
    EXPECT_FALSE(json.contains("nokey"));
}

TEST(Json, LookupFundamental) {
    const auto json = nova::json(input);

    EXPECT_EQ(json.lookup<int>("key"), 1);
    EXPECT_EQ(json.lookup<short>("key"), 1);
    EXPECT_EQ(json.lookup<int>("sub.int"), 9);
    EXPECT_EQ(json.lookup<float>("sub.float"), 9.9F);
    EXPECT_EQ(json.lookup<bool>("sub.boolean"), true);

    const auto& xs = json.at("list");
    EXPECT_EQ(xs.lookup<int>("0"), 1);
    EXPECT_EQ(xs.lookup<int>("1"), 2);
    EXPECT_EQ(xs.lookup<int>("2"), 3);
}

TEST(Json, LookupCompound) {
    using namespace testing;

    const auto json = nova::json(input);

    EXPECT_THAT(
        json.lookup<nlohmann::json::array_t>("list"),
        ElementsAre(1, 2, 3)
    );
}

TEST(Json, LookupDefault) {
    using namespace testing;
    using Array = nlohmann::json::array_t;

    const auto json = nova::json(input);

    EXPECT_EQ(json.lookup<int>("nokey", 1), 1);
    EXPECT_THAT(json.lookup<Array>("nolist", Array{ 1, 2 }), ElementsAre(1, 2));
}
