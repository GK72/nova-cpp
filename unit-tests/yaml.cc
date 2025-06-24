#include "nova/error.hh"
#include "nova/yaml.hh"

#include "test_utils.hh"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <tuple>

constexpr auto data = R"(
int: 9
string: bla
float: 9.9
bool: true
root:
  key: string
array:
  - elem1
  - elem2
  - elem3:
      inner: 1
  - inner1: 2
    inner2: 3
)";

TEST(Yaml, ConstructFromYamlObject) {
    const auto doc = nova::yaml(data);
    const auto inner = doc.at("root");
    EXPECT_EQ(inner.lookup<std::string>("key"), "string");
}

TEST(Yaml, MissingKey) {
    const auto doc = nova::yaml(data);

    EXPECT_THROWN_MESSAGE(
        std::ignore = doc.lookup<int>("nonexistent"),
        "Invalid `.*` in YAML document"
    );

    EXPECT_THROWN_MESSAGE(
        std::ignore = doc.lookup<int>("nonexistent.again"),
        "Invalid `.*` in YAML document"
    );
}

TEST(Yaml, DefaultValueLookup) {
    const auto doc = nova::yaml(data);
    EXPECT_EQ(doc.lookup<int>("int", 6), 9);
    EXPECT_EQ(doc.lookup<int>("noInt", 6), 6);
}

TEST(Yaml, FundamentalTypes) {
    const auto doc = nova::yaml(data);

    EXPECT_EQ(doc.lookup<int>("int"), 9);
    EXPECT_EQ(doc.lookup<std::string>("string"), "bla");
    EXPECT_FLOAT_EQ(doc.lookup<float>("float"), 9.9f);
    EXPECT_EQ(doc.lookup<bool>("bool"), true);
    EXPECT_EQ(doc.lookup<std::string>("root.key"), "string");

    EXPECT_THROWN_MESSAGE(
        std::ignore = doc.lookup<int>("root.key"),
        "error at line 1, column 1: bad conversion"
    );
}

TEST(Yaml, Arrays) {
    const auto doc = nova::yaml(data);
    const auto array = doc.lookup<std::vector>("array");
    EXPECT_EQ(array[0].as<std::string>(), "elem1");
    EXPECT_EQ(array[1].as<std::string>(), "elem2");
    EXPECT_EQ(doc.lookup<std::string>("array.0"), "elem1");
    EXPECT_EQ(doc.lookup<std::string>("array.1"), "elem2");

    EXPECT_THROWN_MESSAGE(
        std::ignore = doc.lookup<std::string>("array.ba"),
        "Invalid `.*` in YAML document"
    );

    EXPECT_EQ(array[2].lookup<int>("elem3.inner"), 1);
    EXPECT_EQ(array[3].lookup<int>("inner1"), 2);
    EXPECT_EQ(array[3].lookup<int>("inner2"), 3);
}

TEST(Yaml, Objects) {
    const auto doc = nova::yaml(data);
    const auto object = doc.lookup<std::map>("array.3");
    EXPECT_EQ(object.at("inner1").as<int>(), 2);
    EXPECT_EQ(object.at("inner2").as<int>(), 3);
}

TEST(Yaml, Dump) {
    const auto doc = nova::yaml(data);
    constexpr auto ref = R"(int: 9
string: bla
float: 9.9
bool: true
root:
  key: string
array:
  - elem1
  - elem2
  - elem3:
      inner: 1
  - inner1: 2
    inner2: 3)";
    EXPECT_EQ(doc.dump(), ref);
}
