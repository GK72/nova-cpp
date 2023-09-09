#include <gmock/gmock.h>

#include "nova/units.h"

using namespace nova::units;
using namespace literals;

TEST(Units, ConversionsImplicitToSmaller) {
    EXPECT_EQ(bit(1_byte),    8_bit);
    EXPECT_EQ(bit(1_kB)  , 8192_bit);
}

TEST(Units, ConversionsExplicitToLarger) {
    EXPECT_EQ(unit_cast<byte>( 9_bit), 1_byte);
    EXPECT_EQ(unit_cast<byte>(15_bit), 1_byte);
    EXPECT_EQ(unit_cast<byte>(16_bit), 2_byte);
}

TEST(Units, ConversionsConvertible) {
    static_assert(!std::is_convertible_v<byte, kByte>);
    static_assert(std::is_convertible_v<kByte, byte>);
}

TEST(Units, ConversionsImplicitBetweenRepresentations) {
    EXPECT_EQ(unit<int>(8), unit<long>(8));

    EXPECT_EQ( (unit<int, std::ratio<1000>>   (8)), (unit<long,   std::ratio<1, 1000>>(8'000'000)) );
    EXPECT_EQ( (unit<int, std::ratio<1000>>   (8)), (unit<double, std::ratio<1, 1000>>(8e6)      ) );
    EXPECT_EQ( (unit<int, std::ratio<1, 1000>>(8)), (unit<double>                     (8e-3)     ) );
}

TEST(Units, HelperTypes) {
    EXPECT_EQ(   8_bit , 1_byte);
    EXPECT_EQ(1024_byte, 1_kB);
    EXPECT_EQ(1024_kB  , 1_MB);
    EXPECT_EQ(1024_MB  , 1_GB);
    EXPECT_EQ(1024_GB  , 1_TB);

    EXPECT_EQ(1_byte, 8_bit);
    EXPECT_EQ(1_kB  , 1024_byte);
    EXPECT_EQ(1_MB  , 1024_kB);
    EXPECT_EQ(1_GB  , 1024_MB);
    EXPECT_EQ(1_TB  , 1024_GB);
}

TEST(Units, RelationalOperatorsCommonType) {
    EXPECT_NE(1_byte, 2_byte);
    EXPECT_LT(1_byte, 2_byte);
    EXPECT_LE(1_byte, 2_byte);
    EXPECT_LE(1_byte, 1_byte);

    EXPECT_NE(2_byte, 1_byte);
    EXPECT_GT(2_byte, 1_byte);
    EXPECT_GE(2_byte, 1_byte);
    EXPECT_GE(2_byte, 2_byte);
}

TEST(Units, RelationalOperatorsDifferentTypes) {
    EXPECT_NE(1_bit, 1_byte);
    EXPECT_LT(1_bit, 1_byte);
    EXPECT_LE(1_bit, 1_byte);
    EXPECT_LE(8_bit, 1_byte);

    EXPECT_NE(1_byte, 1_bit);
    EXPECT_GT(1_byte, 1_bit);
    EXPECT_GE(1_byte, 1_bit);
    EXPECT_GE(1_byte, 8_bit);

    EXPECT_NE(1_byte, 9_bit);
    EXPECT_LT(1_byte, 9_bit);
    EXPECT_LE(1_byte, 9_bit);

    EXPECT_NE(9_bit, 1_byte);
    EXPECT_GE(9_bit, 1_byte);
    EXPECT_GT(9_bit, 1_byte);
}

TEST(Units, ArithmeticOperatorsMultiplication) {
    EXPECT_EQ(12_byte * 2 , 24_byte);
    EXPECT_EQ( 2 * 3_byte ,  6_byte);
}

TEST(Units, ArithmeticOperatorsDivision) {
    EXPECT_EQ(12_byte  / 2,  6_byte);
    EXPECT_EQ( 4_byte  / 2, 16_bit);
}

TEST(Units, ArithmeticOperatorsModulo) {
    EXPECT_EQ(14_bit  % 8    , 6_bit);
    EXPECT_EQ( 4_byte % 6_bit, 2_bit);
}

TEST(Units, ArithmeticOperatorsAddition) {
    EXPECT_EQ(12_byte + 3_byte, 15_byte);
    EXPECT_EQ(12_byte - 3_byte,  9_byte);
}

TEST(Units, ArithmeticOperatorsSubstraction) {
    EXPECT_EQ(12_byte + 2_bit , 98_bit);
    EXPECT_EQ(12_byte - 2_bit , 94_bit);
}

TEST(Units, ArithmeticOperatorsMemberOperators) {
    auto x = 8_byte;

    EXPECT_EQ( (x += 2_byte), 10_byte);
    EXPECT_EQ( (x -= 2_byte),  8_byte);
    EXPECT_EQ( (x *= 2)     , 16_byte);
    EXPECT_EQ( (x /= 4)     ,  4_byte);

    EXPECT_EQ(++x, 5_byte);
    EXPECT_EQ(--x, 4_byte);

    EXPECT_EQ(x++, 4_byte);
    EXPECT_EQ(x  , 5_byte);
    EXPECT_EQ(x--, 5_byte);
    EXPECT_EQ(x  , 4_byte);
}

TEST(Units, MemberFunctions) {
    auto x = 8_byte;

    EXPECT_EQ(x.count(), 8);

    EXPECT_EQ(byte::zero().count(), 0);
    EXPECT_EQ(byte::max().count() , std::numeric_limits<long long>::max());
    EXPECT_EQ(byte::min().count() , std::numeric_limits<long long>::min());

    EXPECT_EQ(unit<double>::min().count(), std::numeric_limits<double>::lowest());
}
