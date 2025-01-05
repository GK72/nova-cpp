#include "nova/data.hh"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <array>
#include <cstdint>
#include <cstddef>
#include <string_view>

using namespace std::literals;

TEST(DataView, FromString) {
    static constexpr auto data = "\x01\x02"sv;
    const auto view_be = nova::data_view(data);
    const auto view_le = nova::data_view_le(data);
    EXPECT_EQ(view_be.as_number<std::uint16_t>(0), 258);
    EXPECT_EQ(view_le.as_number<std::uint16_t>(0), 513);
}

TEST(DataView, FromArray) {
    static constexpr auto data = std::to_array<unsigned char>({ 0x01, 0x02 });
    const auto view_be = nova::data_view(data);
    const auto view_le = nova::data_view_le(data);
    EXPECT_EQ(view_be.as_number<std::uint16_t>(0), 258);
    EXPECT_EQ(view_le.as_number<std::uint16_t>(0), 513);
}

TEST(DataView, FromPtr) {
    static constexpr auto data = "blah";
    const auto view = nova::data_view(data, 4);
    EXPECT_EQ(view.as_string(), "blah"sv);
}

TEST(DataView, InterpretAsNumber_NonStdLength) {
    static constexpr auto data = std::to_array<unsigned char>({ 0x00, 0x01, 0x02, 0x03 });
    const auto view_be = nova::data_view(data);
    const auto view_le = nova::data_view_le(data);
    EXPECT_EQ(view_be.as_number(0, 3), 258);
    EXPECT_EQ(view_le.as_number(0, 3), (2 << 16) + (1 << 8));
}

TEST(DataView, InterpretAsNumber_Indexed) {
    static constexpr auto data = std::to_array<unsigned char>({ 0x00, 0x01, 0x02 });
    const auto view_be = nova::data_view(data);
    const auto view_le = nova::data_view_le(data);
    EXPECT_EQ(view_be.as_number<std::uint16_t>(1), 258);
    EXPECT_EQ(view_le.as_number<std::uint16_t>(1), 513);
}

TEST(DataView, InterpretAsString) {
    static constexpr auto data = "\x61\x62\x63"sv;
    const auto view = nova::data_view(data);
    EXPECT_EQ(view.as_string(0, 3), "abc");
}

TEST(DataView, InterpretAsDynamicString) {
    static constexpr auto data = "\x04\x61\x62\x63\x64\x65"sv;
    EXPECT_EQ(nova::data_view(data).as_dyn_string(0), "abcd");
}

TEST(DataView, SubView) {
    static constexpr auto data = std::to_array<unsigned char>({ 0x01, 0x02, 0x03, 0x04, 0x05 });
    EXPECT_EQ(nova::data_view(data).subview(3).as_number<std::uint8_t>(0), 4);
}

TEST(DataView, SubView_Length) {
    static constexpr auto data = std::to_array<unsigned char>({ 0x01, 0x02, 0x03, 0x04, 0x05 });
    EXPECT_EQ(nova::data_view(data).subview(3, 1).size(), 1);
}

TEST(DataView, ToHexString) {
    static constexpr auto data = "Hello Nova"sv;
    EXPECT_EQ(
        nova::data_view(data).as_hex_string(),
        "48656c6c6f204e6f7661"
    );
}

TEST(DataView, ToVec) {
    static constexpr auto data = "\x00\x61"sv;
    EXPECT_EQ(
        nova::data_view(data).to_vec(),
        ( std::vector<std::byte>{
            std::byte { 0x00 },
            std::byte { 0x61 }
        } )
    );
}

TEST(DataView, ErrorOutOfBounds) {
    static constexpr auto data = std::to_array<unsigned char>({ 0x01, 0x02 });

    EXPECT_THAT(
        []{ std::ignore = nova::data_view(data).as_number(1, 2); },
        testing::ThrowsMessage<nova::exception>(
            testing::HasSubstr("Out of bounds access: Pos=1 Len=2 End=3 (Size=2)")
        )
    );
}
