#include <cstdint>
#include <gtest/gtest.h>

#include "nova/data.hh"
#include "nova/utils.hh"

#include <array>
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

TEST(DataView, InterpretAsNumberIndexed) {
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

TEST(DataView, HexDump) {
    static constexpr auto data = "Hello Nova"sv;
    EXPECT_EQ(
        fmt::format("{}", nova::data_view(data).to_hex()),
        fmt::format("{}0000: 48 65 6c 6c 6f 20 4e 6f 76 61", nova::NewLine)
    );
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
    try {
        std::ignore = nova::data_view(data).as_number(1, 2);
    }
    catch (const nova::out_of_data_bounds& ex) {
        EXPECT_EQ(ex.what(), "Pos: 1, Len: 2, Size: 2 (End: 3)"sv);
    }
}

TEST(Serialization, Serializer_1byte_BigEndian) {
    auto ser = nova::serializer{ 1 };
    ser(std::uint8_t{ 9 });
    EXPECT_EQ(nova::data_view{ ser.data() }.as_hex_string(), "09");
}

TEST(Serialization, Serializer_2bytes_BigEndian) {
    auto ser = nova::serializer{ 2 };
    ser(std::uint16_t{ 256 + 255 });
    EXPECT_EQ(nova::data_view{ ser.data() }.as_hex_string(), "01ff");
}

TEST(Serialization, Serializer_4bytes_BigEndian) {
    auto ser = nova::serializer{ 4 };
    ser(std::uint32_t{ 16909060 });
    EXPECT_EQ(nova::data_view{ ser.data() }.as_hex_string(), "01020304");
}

TEST(Serialization, Serializer_8bytes_BigEndian) {
    auto ser = nova::serializer{ 8 };
    ser(std::uint64_t{ 72057594037928191 });
    EXPECT_EQ(nova::data_view{ ser.data() }.as_hex_string(), "01000000" "000000ff");
}

TEST(Serialization, ph) {
    struct data_t {
        std::uint64_t xl;
        std::uint32_t l;
        std::uint16_t m;
        std::uint8_t  s;
    };

    auto ser = nova::serializer{ 8 + 4 + 2 + 1 };

    constexpr auto data = data_t { 1, 1, 1, 1 };

    ser(data.xl);
    ser(data.l);
    ser(data.m);
    ser(data.s);

    EXPECT_EQ(
        nova::data_view(ser.data()).as_hex_string(),
        "00000000" "00000001"
        "00000001"
        "0001"
        "01"
    );
}
