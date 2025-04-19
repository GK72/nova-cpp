#include "nova/data.hh"
#include "test_utils.hh"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <array>
#include <cstdint>
#include <cstddef>
#include <string>
#include <string_view>

using namespace nova::literals;
using namespace std::literals;

TEST(CharTraitsByte, Compare) {
    using Trait = std::char_traits<std::byte>;

    const auto xs  = std::to_array<std::byte>({
        std::byte{ 0x01 },
        std::byte{ 0x02 },
        std::byte{ 0x03 },
    });

    const auto ys  = std::to_array<std::byte>({
        std::byte{ 0x02 },
        std::byte{ 0x03 },
        std::byte{ 0x04 },
    });

    EXPECT_EQ(Trait::compare(&xs[0], &ys[0], 3), -1);
    EXPECT_EQ(Trait::compare(&ys[0], &xs[0], 3), 1);
    EXPECT_EQ(Trait::compare(&xs[1], &ys[0], 2), 0);
}

TEST(CharTraitsByte, Find) {
    using Trait = std::char_traits<std::byte>;

    const auto xs  = std::to_array<std::byte>({
        std::byte{ 0x01 },
        std::byte{ 0x02 },
        std::byte{ 0x03 },
    });

    EXPECT_EQ(Trait::find(xs.data(), 3, std::byte{ 0x03 }), &xs[2]);
    EXPECT_EQ(Trait::find(xs.data(), 2, std::byte{ 0x03 }), nullptr);
}

TEST(CharTraitsByte, NotEof) {
    using Trait = std::char_traits<std::byte>;

    const auto x = std::byte{ 1 };
    const auto itx = Trait::to_int_type(x);
    EXPECT_TRUE(std::char_traits<std::byte>::not_eof(itx));
    EXPECT_EQ(std::char_traits<std::byte>::not_eof(itx), 1);

    const auto eof = Trait::eof();
    EXPECT_FALSE(std::char_traits<std::byte>::not_eof(eof));
}

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

TEST(DataView, DataLiteral) {
    EXPECT_EQ("abc"_data.as_hex_string(), "616263");
    EXPECT_EQ("\x00\x01\x02"_data.as_hex_string(), "000102");
}

TEST(DataView, Formatter) {
    static constexpr auto data = "Hello Nova"sv;
    EXPECT_EQ(fmt::format("{}", data), "Hello Nova");

    static const auto bin = "\x00\x01\x02"_data;
    EXPECT_EQ(fmt::format("{}", bin), "x000102");
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

TEST(Serialization, Serializer_1byte_BigEndian) {
    auto ser = nova::serializer_context{ };
    ser(std::uint8_t{ 9 });
    EXPECT_EQ(nova::data_view{ ser.data() }.as_hex_string(), "09");
}

TEST(Serialization, Serializer_2bytes_BigEndian) {
    auto ser = nova::serializer_context{ };
    ser(std::uint16_t{ 256 + 255 });
    EXPECT_EQ(nova::data_view{ ser.data() }.as_hex_string(), "01ff");
}

TEST(Serialization, Serializer_4bytes_BigEndian) {
    auto ser = nova::serializer_context{ };
    ser(std::uint32_t{ 16909060 });
    EXPECT_EQ(nova::data_view{ ser.data() }.as_hex_string(), "01020304");
}

TEST(Serialization, Serializer_8bytes_BigEndian) {
    auto ser = nova::serializer_context{ };
    ser(std::uint64_t{ 72057594037928191 });
    EXPECT_EQ(nova::data_view{ ser.data() }.as_hex_string(), "01000000" "000000ff");
}

TEST(Serialization, Serializer_String) {
    auto ser = nova::serializer_context{ };
    ser("abc"s);
    EXPECT_EQ(nova::data_view{ ser.data() }.as_hex_string(), "616263");
}

TEST(Serialization, Serializer_StringView) {
    auto ser = nova::serializer_context{ };
    ser("abc"sv);
    EXPECT_EQ(nova::data_view{ ser.data() }.as_hex_string(), "616263");
}

struct data_t {
    std::uint64_t xl;
    std::string str;
    std::uint32_t l;
    std::uint16_t m;
    std::uint8_t s;
};

namespace nova {

template <>
struct serializer<data_t> {
    void operator()(serializer_context& ser, const data_t& x) {
        ser(x.xl);
        ser(x.str);
        ser(x.l);
        ser(x.m);
        ser(x.s);
    }
};

} // namespace nova

TEST(Serialization, Serialize_FreeFunction) {
    const auto data = data_t { 1, "abc", 1, 1, 1 };

    EXPECT_EQ(
        nova::data_view(nova::serialize(data)).as_hex_string(),
        "00000000" "00000001"
        "616263"
        "00000001"
        "0001"
        "01"
    );
}

TEST(Data, Identity_DataView_Serialization_BigEndian) {
    constexpr auto x = std::uint16_t{ 333 };
    EXPECT_EQ(nova::data_view_be{ nova::serialize(x) }.as_number<std::uint16_t>(0), x);
}

TEST(Data, StreamBuffer_Write) {
    auto buf = nova::stream_buffer{ 10 };
    EXPECT_EQ(buf.write("Hello"_data), 5);
    EXPECT_EQ(buf.write(" Nova"_data), 5);

    EXPECT_EQ(buf.view().as_string(), "Hello Nova");
}

TEST(Data, StreamBuffer_Consume) {
    auto buf = nova::stream_buffer{ 10 };
    EXPECT_EQ(buf.write("Hello Nova"_data), 10);
    buf.consume(6);
    EXPECT_EQ(buf.view().as_string(), "Nova");
}

TEST(Data, StreamBuffer_WriteConsumeLoop) {
    auto buf = nova::stream_buffer{ 10 };
    EXPECT_EQ(buf.write("Hello "_data), 6);

    auto data = "overflow"_data;
    auto n = buf.write(data);
    EXPECT_EQ(n, 4);
    EXPECT_EQ(buf.view().as_string(), "Hello over");

    buf.consume(4);
    EXPECT_EQ(buf.view().as_string(),     "o over");

    EXPECT_EQ(buf.write(data.subview(n)), 4);
    EXPECT_EQ(buf.view().as_string(), "o overflow");
}

TEST(Data, StreamBuffer_WriteToFull) {
    auto buf = nova::stream_buffer{ 10 };
    EXPECT_EQ(buf.write("Hello Nova"_data), 10);
    EXPECT_EQ(buf.write("a"_data), 0);
    EXPECT_EQ(buf.write("a"_data), 0);
    EXPECT_EQ(buf.write("Hello Nova"_data), 0);
}

TEST(Data, StreamBuffer_ConsumeAll) {
    auto buf = nova::stream_buffer{ 10 };
    EXPECT_EQ(buf.write("Hello Nova"_data), 10);
    EXPECT_EQ(buf.view().as_string(), "Hello Nova");

    buf.consume();
    EXPECT_TRUE(buf.view().empty());
}

TEST(Data, StreamBuffer_ResizingBuffer) {
    const auto data = std::string(256, 'a');
    auto buf = nova::stream_buffer{ 512 };
    EXPECT_EQ(buf.write(nova::data_view(data)), 256);

    EXPECT_EQ(buf.size(), 256);
    EXPECT_EQ(buf.view().size(), 256);
}
