#include <gtest/gtest.h>

#include "nova/error.h"
#include "nova/yaml.h"

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

// TODO: stoi exception
TEST(Yaml, FundamentalTypes) {
    const auto doc = nova::yaml(data);

    EXPECT_EQ(doc.lookup<int>("int"), 9);
    EXPECT_EQ(doc.lookup<std::string>("string"), "bla");
    EXPECT_FLOAT_EQ(doc.lookup<float>("float"), 9.9f);
    EXPECT_EQ(doc.lookup<bool>("bool"), true);
    EXPECT_EQ(doc.lookup<std::string>("root.key"), "string");
    EXPECT_THROW(std::ignore = doc.lookup<int>("root.key"), nova::parsing_error);
}

TEST(Yaml, Arrays) {
    const auto doc = nova::yaml(data);
    const auto array = doc.lookup<std::vector>("array");
    EXPECT_EQ(array[0].as<std::string>(), "elem1");
    EXPECT_EQ(array[1].as<std::string>(), "elem2");
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
