#define NOVA_RUNTIME_ASSERTIONS

#include "test_utils.hh"

#include "nova/data.hh"
#include "nova/units.hh"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <array>
#include <cstdint>
#include <cstddef>
#include <limits>
#include <string>
#include <string_view>
#include <tuple>

using namespace nova::literals;
using namespace nova::units::literals;
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
    EXPECT_EQ(view_be.as_number(nova::extent<std::size_t>{ 0, 3 }), 258);
    EXPECT_EQ(view_le.as_number(0, 3), (2 << 16) + (1 << 8));

    EXPECT_ASSERTION_FAIL(std::ignore = view_be.as_number<std::uint8_t>(0, 2));
}

TEST(DataView, InterpretAsNumber_Indexed) {
    static constexpr auto data = std::to_array<unsigned char>({ 0x00, 0x01, 0x02 });
    const auto view_be = nova::data_view(data);
    const auto view_le = nova::data_view_le(data);
    EXPECT_EQ(view_be.as_number<std::uint16_t>(1), 258);
    EXPECT_EQ(view_le.as_number<std::uint16_t>(1), 513);
}

TEST(DataView, SignedNumbers) {
    EXPECT_EQ(nova::data_view("\x01"sv).as_number<std::int8_t>(0), 1);
    EXPECT_EQ(nova::data_view("\xFF"sv).as_number<std::int8_t>(0), -1);
    EXPECT_EQ(nova::data_view("\x80"sv).as_number<std::int8_t>(0), -128);
    EXPECT_EQ(nova::data_view("\x81"sv).as_number<std::int8_t>(0), -127);
    EXPECT_EQ(nova::data_view("\x82"sv).as_number<std::int8_t>(0), -126);

    EXPECT_EQ(nova::data_view("\x80\x00"sv).as_number<std::int16_t>(0), -32768);
    EXPECT_EQ(nova::data_view("\x80\x01"sv).as_number<std::int16_t>(0), -32767);
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

TEST(DataView, InterpretAsBitPackedNumber_OneByte) {
    static constexpr auto data = std::to_array<unsigned char>({
        0b1100'0001
    });

    const auto view = nova::data_view(data);

    EXPECT_EQ(view.as_number_bit_packed(0, 1), 1);
    EXPECT_EQ(view.as_number_bit_packed(0, 2), 3);

    EXPECT_EQ(view.as_number_bit_packed(1, 1), 1);
    EXPECT_EQ(view.as_number_bit_packed(1, 2), 2);

    EXPECT_EQ(view.as_number_bit_packed(7, 1), 1);
    EXPECT_EQ(view.as_number_bit_packed(3, 5), 1);
}

TEST(DataView, InterpretAsBitPackedNumber_ph) {
    static constexpr auto data = std::to_array<unsigned char>({
        0b1100'0001,
        0b1010'0011
    });
    const auto view = nova::data_view(data);

    // FIXME: Current result is `10` instead of `26`.
    // EXPECT_EQ(view.as_number_bit_packed<std::uint8_t>(4, 8), 26);
    EXPECT_EQ(view.as_number_bit_packed<std::uint16_t>(4, 8), 26);
}

TEST(DataView, InterpretAsBitPackedNumber_MultipleBytes) {
    static constexpr auto data = std::to_array<unsigned char>({
        0b1100'0001,
        0b1010'0011
    });

    const auto view = nova::data_view(data);

    EXPECT_EQ(view.as_number_bit_packed(8, 1), 1);
    EXPECT_EQ(view.as_number_bit_packed(8, 3), 5);
    EXPECT_EQ(view.as_number_bit_packed(8, 8), 163);

    EXPECT_EQ(view.as_number_bit_packed(4, 4), 1);
    EXPECT_EQ(view.as_number_bit_packed(4, 5), 3);
    EXPECT_EQ(view.as_number_bit_packed(4, 12), 419);
}

TEST(DataView, InterpretAsBitPackedNumber_ExtentOverload) {
    static constexpr auto data = std::to_array<unsigned char>({
        0b1100'0001,
        0b1010'0011,
        0x02,
        0x03,
        0x04,
        0x05,
        0x06,
        0x07
    });

    const auto view = nova::data_view(data);
    constexpr auto byte_pos = nova::units::bytes{ 1 };
    constexpr auto bit_pos = nova::units::bits{ 8 };
    constexpr auto len = nova::units::bits { 3 };
    const auto byte_extent = nova::extent{ byte_pos, len };
    const auto bit_extent = nova::extent{ bit_pos, len };

    EXPECT_EQ(view.as_number<std::size_t>(byte_extent), 5);
    EXPECT_EQ(view.as_number<std::size_t>(bit_extent), 5);
    EXPECT_EQ(view.as_number<std::size_t>(nova::extent{ 1_byte, 3_bit }), 5);

    EXPECT_EQ(view.as_number<std::size_t>(nova::extent{ 6_byte, 2_byte }), 1543);
    EXPECT_EQ(view.as_number<std::uint16_t>(6), 1543);
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

TEST(DataView, ErrorOutOfBounds_Bit) {
    static constexpr auto data = std::to_array<unsigned char>({
        0b1100'0001
    });

    EXPECT_THAT(
        []{ std::ignore = nova::data_view(data).as_number_bit_packed(0, 9); },
        testing::ThrowsMessage<nova::exception>(
            testing::HasSubstr("Out of bounds access: Pos=0 Len=9 End=9 (Size=8) (bits)")
        )
    );

    EXPECT_THAT(
        []{ std::ignore = nova::data_view(data).as_number_bit_packed(1, 8); },
        testing::ThrowsMessage<nova::exception>(
            testing::HasSubstr("Out of bounds access: Pos=1 Len=8 End=9 (Size=8) (bits)")
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

TEST(Data, StreamBuffer_LimitedSize) {
    EXPECT_THROWN_MESSAGE(
        nova::stream_buffer{ static_cast<nova::stream_buffer<>::difference_type>(std::numeric_limits<int>::max()) + 1 },
        "Maximum buffer size.*is over the limit"
    );
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

    buf.consume(n);
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
    EXPECT_EQ(buf.size(), buf.view().size());
    EXPECT_EQ(buf.view().as_string(), data);
}
