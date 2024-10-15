#include <gtest/gtest.h>

#include "nova/parse.h"

#include <chrono>
#include <string_view>

using namespace std::chrono_literals;
using namespace std::string_view_literals;

namespace nova {
    using namespace alpha;
}

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

    // Separate implementation due to floating-point numbers are not yet
    // supported in libcxx by `std::from_chars()`.

    EXPECT_EQ(nova::to_number<float>("bla").error(), nova::parse_error::invalid_argument);
    EXPECT_EQ(nova::to_number<float>("1.1e+500").error(), nova::parse_error::out_of_range);
}

TEST(Parse, ToChrono_Types) {
    EXPECT_EQ(nova::to_chrono<std::chrono::nanoseconds> ("72ns").value(), 72ns);
    EXPECT_EQ(nova::to_chrono<std::chrono::microseconds>("72us").value(), 72us);
    EXPECT_EQ(nova::to_chrono<std::chrono::milliseconds>("72ms").value(), 72ms);
    EXPECT_EQ(nova::to_chrono<std::chrono::seconds>     ("72s").value() , 72s);
    EXPECT_EQ(nova::to_chrono<std::chrono::minutes>     ("2min").value(), 2min);
    EXPECT_EQ(nova::to_chrono<std::chrono::hours>       ("72h").value() , 72h);
    EXPECT_EQ(nova::to_chrono<std::chrono::days>        ("72d").value() , std::chrono::days{ 72 });
    EXPECT_EQ(nova::to_chrono<std::chrono::weeks>       ("72w").value() , std::chrono::weeks{ 72 });
    EXPECT_EQ(nova::to_chrono<std::chrono::months>      ("72M").value() , std::chrono::months{ 72 });
    EXPECT_EQ(nova::to_chrono<std::chrono::years>       ("72y").value() , std::chrono::years{ 72 });
}

TEST(Parse, ToChrono_Conversions) {
    EXPECT_EQ(nova::to_chrono<std::chrono::microseconds>("1ms").value(), 1000us);
    EXPECT_EQ(nova::to_chrono<std::chrono::microseconds>("1000ns").error(), nova::parse_error::lossy_conversion);
    EXPECT_EQ(nova::to_chrono<std::chrono::microseconds>("1001ns").error(), nova::parse_error::lossy_conversion);
    EXPECT_EQ(nova::to_chrono<std::chrono::nanoseconds>("1000y").error(), nova::parse_error::out_of_range);
}

TEST(Parse, ToNumberMetric) {
    EXPECT_EQ(nova::to_number_metric<int>("1k").value(), 1000);
    EXPECT_EQ(nova::to_number_metric<int>("1M").value(), 1'000'000);
    EXPECT_EQ(nova::to_number_metric<int>("1G").value(), 1'000'000'000);
    EXPECT_EQ(nova::to_number_metric<int>("1000m").value(), 1);
    EXPECT_FLOAT_EQ(nova::to_number_metric<float>("1m").value(), 0.001F);
    EXPECT_FLOAT_EQ(nova::to_number_metric<float>("1u").value(), 0.000'001F);
    EXPECT_FLOAT_EQ(nova::to_number_metric<float>("1n").value(), 0.000'000'001F);

    // TODO(feat): safe cast
    // EXPECT_EQ(nova::to_number_human<int>("1T").value(), 1'000'000'000'000);
    // EXPECT_EQ(nova::to_number_human<int>("1m").value(), 0.001F);
}
