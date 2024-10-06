#include <gtest/gtest.h>

#include "nova/parse.h"

#include <string_view>

using namespace std::string_view_literals;

TEST(Parse, NumAndSuffix) {
    EXPECT_EQ(nova::detail::split_num_n_suffix<int>("1000abc").value(), std::make_pair(1000, "abc"sv));
    EXPECT_EQ(nova::detail::split_num_n_suffix<int>("1000abc bla").value(), std::make_pair(1000, "abc bla"sv));
}

TEST(Parse, ToNumber) {
    EXPECT_EQ(nova::to_number<int>("a").error(), nova::parse_error::invalid_argument);
    EXPECT_EQ(nova::to_number<unsigned int>("-1").error(), nova::parse_error::invalid_argument);
    EXPECT_EQ(nova::to_number<char>("100000").error(), nova::parse_error::out_of_range);
    EXPECT_EQ(nova::to_number<int>("1").value(), 1);
    EXPECT_EQ(nova::to_number<int>("-1").value(), -1);
    EXPECT_FLOAT_EQ(nova::to_number<float>("1.23").value(), 1.23F);
    EXPECT_FLOAT_EQ(nova::to_number<float>("1.230").value(), 1.23F);
}
