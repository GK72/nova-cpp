#include <libnova/parse.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <limits>
#include <string_view>

using namespace std::chrono_literals;
using namespace std::string_view_literals;

TEST(Parse, ParseErrorFormatter) {
    EXPECT_EQ(fmt::format("{}", nova::parse_error::invalid_argument), "Parse error (invalid argument)");
    EXPECT_EQ(fmt::format("{}", nova::parse_error::out_of_range), "Parse error (out of range)");
}

TEST(Parse, Detail_SafeMultiply) {
    constexpr auto maxchar = std::numeric_limits<char>::max();
    EXPECT_EQ(nova::detail::safe_multiply<char>(1, maxchar).value(), maxchar);
    EXPECT_EQ(nova::detail::safe_multiply<char>(2, maxchar + 1).error(), nova::parse_error::out_of_range);
    EXPECT_EQ(nova::detail::safe_multiply<float>(3.40282347E+38F, 10).error(), nova::parse_error::out_of_range);
}

TEST(Parse, NumAndSuffix) {
    EXPECT_EQ(nova::detail::split_num_n_suffix<int>("1000").value(), std::make_pair(1000, ""sv));
    EXPECT_EQ(nova::detail::split_num_n_suffix<int>("1000abc").value(), std::make_pair(1000, "abc"sv));
    EXPECT_EQ(nova::detail::split_num_n_suffix<int>("1000abc bla").value(), std::make_pair(1000, "abc bla"sv));

    const auto f = nova::detail::split_num_n_suffix<float>("1.1e+10").value();
    EXPECT_FLOAT_EQ(f.first, 1.1e+10F);
    EXPECT_EQ(f.second, ""sv);
}

TEST(Parse, ToNumber) {
    EXPECT_EQ(nova::to_number<int>("a").error(), nova::parse_error::invalid_argument);
    EXPECT_EQ(nova::to_number<unsigned int>("-1").error(), nova::parse_error::invalid_argument);
    EXPECT_EQ(nova::to_number<char>("100000").error(), nova::parse_error::out_of_range);
    EXPECT_EQ(nova::to_number<int>("1").value(), 1);
    EXPECT_EQ(nova::to_number<int>("-1").value(), -1);
    EXPECT_FLOAT_EQ(nova::to_number<float>("1.23").value(), 1.23F);
    EXPECT_FLOAT_EQ(nova::to_number<float>("1.230").value(), 1.23F);
    EXPECT_FLOAT_EQ(nova::to_number<float>("1.001e+2").value(), 100.1F);
    EXPECT_EQ(nova::to_number<float>("bla").error(), nova::parse_error::invalid_argument);
    EXPECT_EQ(nova::to_number<float>("1.1e+500").error(), nova::parse_error::out_of_range);
}

TEST(Parse, ToNumberMetric) {
    ASSERT_FALSE(nova::to_number<std::int32_t>("1E").has_value());
    EXPECT_EQ(nova::to_number<std::int32_t>("1E").error(), nova::parse_error::out_of_range);

    EXPECT_EQ(nova::to_number<std::int32_t>("1000m").error(), nova::parse_error::invalid_argument);

    EXPECT_EQ(nova::to_number<std::int64_t>("1E").value(), 1'000'000'000'000'000'000);
    EXPECT_EQ(nova::to_number<std::int64_t>("1P").value(), 1'000'000'000'000'000);
    EXPECT_EQ(nova::to_number<std::int64_t>("1T").value(), 1'000'000'000'000);
    EXPECT_EQ(nova::to_number<std::int32_t>("3G").error(), nova::parse_error::out_of_range);
    EXPECT_EQ(nova::to_number<std::int32_t>("1G").value(), 1'000'000'000);
    EXPECT_EQ(nova::to_number<std::int32_t>("1M").value(), 1'000'000);
    EXPECT_EQ(nova::to_number<std::int32_t>("1k").value(), 1000);
    EXPECT_FLOAT_EQ(nova::to_number<float>("1m").value(), 0.001F);
    EXPECT_FLOAT_EQ(nova::to_number<float>("1u").value(), 0.000'001F);
    EXPECT_FLOAT_EQ(nova::to_number<float>("1n").value(), 0.000'000'001F);
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
    EXPECT_EQ(nova::to_chrono<std::chrono::microseconds>("1000ns").value(), 1us);
    EXPECT_EQ(nova::to_chrono<std::chrono::microseconds>("1001ns").value(), 1us);
    EXPECT_EQ(nova::to_chrono<std::chrono::nanoseconds>("1000y").error(), nova::parse_error::out_of_range);
}
