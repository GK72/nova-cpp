#include "nova/flat_map.hh"

#include <gmock/gmock.h>

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

using IntMap = nova::flat_map<int, int>;

TEST(FlatMap, ConstructFromInitializerList) {
    const auto map = IntMap({
        { 2, 4 },
        { 3, 6 },
    });

    EXPECT_EQ(map.size(), 2);
    EXPECT_EQ(map.keys(), ( std::vector{ 2, 3 } ));
}

TEST(FlatMap, Observers) {
    auto map = IntMap();

    EXPECT_EQ(map.size(), 0);
    EXPECT_TRUE(map.empty());
    EXPECT_TRUE(map.keys().empty());
    EXPECT_TRUE(map.values().empty());
}

TEST(FlatMap, Iterators) {
    static_assert(std::is_same_v<IntMap::iterator::iterator_category, std::random_access_iterator_tag>);

    auto map = IntMap();

    auto begin = std::begin(map);
    auto end = std::end(map);
    auto rbegin = std::rbegin(map);
    auto rend = std::rend(map);

    const auto cbegin = std::begin(map);
    const auto cend = std::end(map);
    const auto ccbegin = std::cbegin(map);
    const auto ccend = std::cend(map);

    const auto crbegin = std::rbegin(map);
    const auto crend = std::rend(map);
    const auto ccrbegin = std::crbegin(map);
    const auto ccrend = std::crend(map);

    EXPECT_EQ(begin, end);
    EXPECT_EQ(cbegin, cend);
    EXPECT_EQ(ccbegin, ccend);
    EXPECT_EQ(rbegin, rend);
    EXPECT_EQ(crbegin, crend);
    EXPECT_EQ(ccrbegin, ccrend);

    for ([[maybe_unused]] const auto& [key, value] : map) {
    }
}

TEST(FlatMap, Insertion) {
    auto map = IntMap();
    map.insert({ 2, 4 });
    map.insert({ 1, 2 });

    EXPECT_EQ(map.values(), std::vector<int>({ 2, 4 }));

    map.insert({ 1, 3 });
    EXPECT_EQ(map.values(), std::vector<int>({ 2, 4 }));
}

TEST(FlatMap, StdAlgorithmConform) {
    const auto map = IntMap{
        { 1, 10 },
        { 2, 22 },
        { 3, 31 }
    };

    [[maybe_unused]] const auto _ = std::find_if(std::begin(map), std::end(map), [](const auto& p) { return p.second == 22; });
}

TEST(FlatMap, At) {
    const auto map = IntMap({
        { 2, 4 },
        { 3, 6 },
    });

    EXPECT_EQ(map.at(2), 4);
    EXPECT_THROW(std::ignore = map.at(4), std::out_of_range);
}

TEST(FlatMap, SubscriptOperator) {
    auto map = IntMap();

    map[4] = 2;
    map[2] = 4;
    map[1] = 3;
    map[3] = 1;

    EXPECT_EQ(map[1], 3);
    EXPECT_EQ(map[2], 4);
    EXPECT_EQ(map[3], 1);
    EXPECT_EQ(map[4], 2);

    map[4] = 11;
    EXPECT_EQ(map[4], 11);
}

TEST(FlatMap, StaticMap) {
    using Dict = nova::static_map<std::string_view, std::string_view, 2>;

    constexpr auto map = Dict({
        { "a", "something" },
        { "b", "another something" }
    });

    EXPECT_EQ(map.size(), 2);
    EXPECT_EQ(map.keys(), ( std::array<std::string_view, 2>({ "a", "b" }) ));
}
