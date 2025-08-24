/**
 * Part of Nova C++ Library.
 *
 * Handling binary data; serialization and deserialization.
 *
 * - Deserialization: `data_view` for safely interpreting binary data.
 * - Serialization:
 *   - `serializer_context` class for low-level handling
 *   - `serialize(x)` free function for convenience
 * - Stream buffer
 *
 * A `serializer<T>` specialization is required for type `T` to be able to
 * serialize it.
 *
 * ```cpp
 * struct data {
 *     std::uint8_t member;
 * };
 *
 * template <>
 * struct serializer<data>
 *     void operator()(serializer_context& ser, const data& x) {
 *         ser(x.member);
 *     }
 * };
 * ```
 */

#pragma once

#include <libnova/error.hpp>
#include <libnova/type_traits.hpp>
#include <libnova/types.hpp>
#include <libnova/units.hpp>
#include <libnova/utils.hpp>

#include <fmt/core.h>
#include <fmt/format.h>

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits>
#include <span>
#include <streambuf>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace std {

template <>
struct char_traits<std::byte> {
    using char_type = std::byte;

    // An integer type the can represent a character of `char_type` values, as
    // well as an end-of-file value, eof().
    using int_type = short;

    // An integer type that can represent positions in a stream.
    using pos_type = streampos;

    // An integer type that can represent offsets between positions in a stream.
    using off_type = streamoff;

    // A type that represents the conversion state in for multibyte characters in a stream.
    // Reference: https://en.cppreference.com/w/c/string/multibyte/mbstate_t
    using state_type = mbstate_t;

    static constexpr void assign(char_type& lhs, char_type rhs) noexcept {
        lhs = rhs;
    }

    static constexpr auto eq(char_type lhs, char_type rhs) noexcept -> bool {
        return lhs == rhs;
    }

    static constexpr auto lt(char_type lhs, char_type rhs) noexcept -> bool {
        return lhs < rhs;
    }

    static constexpr auto compare(const char_type* lhs, const char_type* rhs, size_t n) -> int {
        if (n == 0) {
            return 0;
        }

        for (size_t i = 0; i < n; ++i) {
            if (lt(lhs[i], rhs[i])) {                                                               // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                return -1;
            }
            if (lt(rhs[i], lhs[i])) {                                                               // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                return 1;
            }
        }

        return 0;
    }

    static constexpr auto length(const char_type* ch) -> size_t NOVA_DELETE("Cannot tell the length of a byte array given only a pointer");

    static constexpr auto find(const char_type* where, size_t n, const char_type& what) -> const char_type* {
        for (size_t i = 0; i < n; ++i) {
            if (*where == what) {
                return where;
            }
            advance(where, 1);
        }
        return nullptr;
    }

    static auto move(char_type* dest, const char_type* src, size_t n) -> char_type* {
        memmove(dest, src, n);
        return dest;
    }

    static auto copy(char_type* dest, const char_type* src, size_t n) -> char_type* {
        memcpy(dest, src, n);
        return dest;
    }

    static auto assign(char_type* ptr, size_t n, char_type ch) -> char_type* {
        memset(ptr, to_integer<int>(ch), n);
        return ptr;
    }

    static constexpr auto eof() noexcept -> int_type { return -1; }

    static constexpr auto not_eof(int_type ch) noexcept -> int_type {
        return eq_int_type(ch, eof())
             ? static_cast<int_type>(~eof())
             : ch;
    }

    // If there is no equivalent char_type value the result is unspecified.
    static constexpr auto to_char_type(int_type ch) noexcept -> char_type {
      return static_cast<char_type>(ch);
    };

    static constexpr auto to_int_type(char_type ch) noexcept -> int_type {
        return static_cast<int_type>(ch);
    }

    static constexpr auto eq_int_type(int_type lhs, int_type rhs) noexcept -> bool {
        return lhs == rhs;
    }

};

} // namespace std

namespace nova::detail {

    struct data_cursor {
        std::size_t pos;
        std::size_t length;
        std::size_t size;
    };

} // namespace nova::detail

template <>
class fmt::formatter<nova::detail::data_cursor> {
public:
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template <typename FmtContext>
    constexpr auto format(const nova::detail::data_cursor& cursor, FmtContext& ctx) const {
        return fmt::format_to(
            ctx.out(),
            "Pos={} Len={} End={} (Size={})",
            cursor.pos,
            cursor.length,
            cursor.pos + cursor.length,
            cursor.size
        );
    }
};
namespace nova {

using bytes = std::vector<std::byte>;

template <typename T>
concept binary_interpretable =
    std::is_same_v<T, char>
    || std::is_same_v<T, unsigned char>
    || std::is_same_v<T, std::uint8_t>
    || std::is_same_v<T, std::byte>;

enum class endian : std::uint8_t {
    big,
    little,
};

namespace detail {

/**
 * @brief   A binary data view on a range.
 *
 * Interprets binary data in a type safe manner either in big or little endian.
 *
 * NOTE: it is a non-owning view, it is the caller's responsibility to make sure
 * the `data_view` does not outlive the data it refers to.
 *
 * @throws  All accessor member functions throw if `RuntimeBoundCheck`
 *          is turned on, else calls to `assert` (debug mode check)
 */
template <endian Endianness = endian::big, bool RuntimeBoundCheck = true>
class data_view {
    static constexpr auto Byte = 8;

    template <typename R>
    using Measure = units::measure<units::data_volume, long long, R>;

public:

    template <typename Range>
        requires binary_interpretable<std::remove_cv_t<typename Range::value_type>>
            and (
                std::contiguous_iterator<typename Range::iterator>
                or std::is_array_v<Range>
            )
    [[nodiscard]]
    data_view(const Range& data)
        : m_data(std::as_bytes(std::span(data)))
    {}

    [[nodiscard]]
    data_view(const void* ptr, std::size_t size)
        : m_data(reinterpret_cast<const std::byte*>(ptr), size)                                     // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast) | Low-level code; it is safe to cast between `const char` and `std::byte`.
    {}

    [[nodiscard]] data_view subview(std::size_t offset) const {
        return data_view(m_data.subspan(offset));
    }

    [[nodiscard]] data_view subview(std::size_t offset, std::size_t length) const {
        return data_view(m_data.subspan(offset, length));
    }

    [[nodiscard]] auto begin() const { return std::begin(m_data); }
    [[nodiscard]] auto begin()       { return std::begin(m_data); }
    [[nodiscard]] auto end()   const { return std::end(m_data); }
    [[nodiscard]] auto end()         { return std::end(m_data); }

    [[nodiscard]] auto span()  const -> std::span<const std::byte> { return m_data; }
    [[nodiscard]] auto empty() const -> bool                       { return m_data.empty(); }
    [[nodiscard]] auto size()  const -> std::size_t                { return m_data.size(); }

    [[nodiscard]] auto ptr()      const -> const std::byte* { return m_data.data(); }
    [[nodiscard]] auto char_ptr() const -> const char*      { return reinterpret_cast<const char*>(m_data.data()); }    // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast) | Low-level code; it is safe to cast between `const char` and `std::byte`.

    /**
     * @brief   Interpret data as number specified by `length`.
     */
    template <typename T = std::size_t>
        requires std::is_integral_v<T>
    [[nodiscard]] auto as_number(std::size_t pos, std::size_t length) const -> T {
        nova_assert(length <= sizeof(T));
        boundary_check(pos, length);

        auto ret = T {};
        for (std::size_t i = 0; i < length; ++i) {
            if constexpr (Endianness == endian::big) {
                ret |= static_cast<T>(std::to_integer<T>(m_data[pos + i]) << (length - i - 1) * Byte);
            }
            else {
                ret |= static_cast<T>(std::to_integer<T>(m_data[pos + i]) << i * Byte);
            }
        }
        return ret;
    }

    /**
     * @brief   Interpret bit-packed data as number specified by `length` from `pos`.
     *
     * Both `pos` and `length` specified as number of bits.
     * The overload `as_number(extent<units::measure<...>>)` offers specifying
     * them in either bytes or bits.
     *
     * Note: Bit-packed deserialization is ONLY supported in big endian mode!
     *
     * Algorithm:
     * - The byte-array (given by `pos` and `length`) is extracted into a single value.
     *   - If the range goes through more bytes than the type is able to represent,
     *     a carry byte is used. For example 8 bits from the 5th bit requires two
     *     bytes to be used.
     * - Trailing (rightmost) bits are shifted off.
     * - Leading bits are masked off.
     * - Carry bits are shifted left by the amount of spillover bits.
     */
    template <typename T = std::size_t>
        requires std::is_integral_v<T>
             and std::is_unsigned_v<T>
    [[nodiscard]] auto as_number_bit_packed(std::size_t pos, std::size_t length) const -> T {
        static_assert(Endianness == endian::big);
        nova_assert(length <= sizeof(T) * Byte);
        boundary_check_bit(pos, length);

        std::size_t start_byte = pos / Byte;
        const std::size_t end_bit = pos + length;
        const std::size_t end_byte = (end_bit + 7) / Byte;

        auto ret = T{};
        auto carry = T{};

        if (end_byte - start_byte > sizeof(T)) {
            carry = std::to_integer<std::uint8_t>(m_data[start_byte]);
            ++start_byte;
        }

        for (std::size_t i = start_byte; i < end_byte; ++i) {
            ret = static_cast<T>(ret << Byte);
            ret |= std::to_integer<std::uint8_t>(m_data[i]);
        }

        const std::size_t trailing_bits = end_byte * Byte - end_bit;
        ret = static_cast<T>(ret >> trailing_bits);

        if (length < sizeof(T) * Byte) {
            T mask = static_cast<T>(1ULL << length) - 1;
            ret &= mask;
        }

        if (carry) {
            const auto spillover_bits = Byte - trailing_bits + (sizeof(T) - 1) * Byte;
            carry = static_cast<T>(carry << spillover_bits);
            ret |= carry;
        }

        return ret;
    }

    /**
     * @brief   Interpret data as number according to the type `T`.
     *
     * This is wrapper around `as_number(pos, len)`.
     */
    template <typename T>
        requires std::is_integral_v<T>
    [[nodiscard]] auto as_number(std::size_t pos) const -> T {
        return as_number<T>(pos, sizeof(T));
    }

    /**
     * @brief   Interpret data as number according to the type `T`.
     */
    template <typename T = std::size_t>
        requires std::is_integral_v<T>
    [[nodiscard]] auto as_number(extent<std::size_t> ex) const -> T {
        return as_number(ex.pos, ex.len);
    }

    /**
     * @brief   Interpret bit-packed data as number specified by
     *          `extent<units::measure<...>>`.
     *
     * Position and length in `extent` does not need to have the same ratio.
     *
     * @param ex    The extent specifing the position and length. Both must
     *              be a measure representing bits or bytes as an integral
     *              type.
     *
     * @tparam T    Return type which must be an unsigned integer and
     *              must be able to represent the requrested length.
     * @tparam R1   Ratio for position in `extent`.
     * @tparam R2   Ratio for length in `extent`.
     *
     * @example
     *
     * ```cpp
     * const auto view = nova::data_view(...);
     * view.as_number<std::size_t>(nova::extent{ 1_byte, 2_bit });
     * ```
     *
     * Note: Bit-packed deserialization is ONLY supported in big endian mode!
     */
    template <typename T = std::size_t, typename R1, typename R2 = R1>
        requires std::is_integral_v<T>
             and std::is_unsigned_v<T>
    [[nodiscard]] auto as_number(extent<Measure<R1>, Measure<R2>> ex) const -> T {
        return as_number_bit_packed(
            static_cast<std::size_t>(units::measure_cast<units::bits>(ex.pos).count()),
            static_cast<std::size_t>(units::measure_cast<units::bits>(ex.len).count())
        );
    }

    /**
     * @brief   Interpret data for the given `length` as a string.
     */
    [[nodiscard]]
    auto as_string(std::size_t pos, std::size_t length) const -> std::string_view {
        boundary_check(pos, length);
        return { reinterpret_cast<const char*>(m_data.data() + pos), length };                      // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast) | Low-level code; it is safe to cast between `const char` and `std::byte`.
    }

    [[nodiscard]]
    auto as_string() const -> std::string_view {
        return as_string(0, size());
    }

    /**
     * @brief   Interpret data as dynamic length string.
     *
     * Length of the string is described in the first `length_bytes` bytes.
     */
    [[nodiscard]]
    auto as_dyn_string(std::size_t pos, std::uint8_t length_bytes = 1) const -> std::string_view {
        const auto str_length = as_number(pos, length_bytes);
        boundary_check(pos, str_length);
        return as_string(pos + length_bytes, str_length);
    }

    [[nodiscard]]
    auto as_hex_string(std::size_t pos, std::size_t length) const -> std::string {
        std::string ret;

        for (const auto& ch : m_data.subspan(pos, length)) {
            ret += fmt::format("{:02x}", ch);
        }

        return ret;
    }

    [[nodiscard]]
    auto as_hex_string() const -> std::string {
        return as_hex_string(0, size());
    }

    [[nodiscard]]
    auto to_vec() const -> bytes {
        auto ret = bytes(size());

        std::ranges::copy(
            std::begin(m_data),
            std::end(m_data),
            std::begin(ret)
        );

        return ret;
    }

private:
    std::span<const std::byte> m_data;

    void boundary_check(std::size_t pos, std::size_t length) const {
        if constexpr (RuntimeBoundCheck) {
            if (size() < pos + length) {
                throw exception(
                    "Out of bounds access: {}",
                    detail::data_cursor{ pos, length, size() }
                );
            }
        }
        else {
            nova_assert(pos + length <= size());
        }
    }

    void boundary_check_bit(std::size_t pos, std::size_t length) const {
        if constexpr (RuntimeBoundCheck) {
            if (size() * Byte < pos + length) {
                throw exception(
                    "Out of bounds access: {} (bits)",
                    detail::data_cursor{ pos, length, size() * Byte }
                );
            }
        }
        else {
            nova_assert(pos + length <= size() * Byte);
        }
    }
};

} // namespace detail

using data_view = detail::data_view<>;
using data_view_be = detail::data_view<endian::big>;
using data_view_le = detail::data_view<endian::little>;

namespace literals {

    [[nodiscard]] inline
    auto operator""_data(const char* str, std::size_t n) noexcept {
        return data_view{ str, n };
    }

} // namespace literals

class serializer_context;

template <typename T>
struct serializer {
    void operator()(serializer_context&, const T&) {
        static_assert(dependent_false<T>, "Cannot serialize the type. Provide a serializer<T> specialization.");
    }
};

/**
 * @brief   Serializer that holds a byte array.
 *
 * The underlying vector holding the bytes is automatically resized with a
 * geometric growth if needed. For performance oriented use cases consider
 * creating the context with a predefined sized to avoid unnecessary
 * reallocations. It will still resize if the preallocation is not large enough.
 */
class serializer_context {
    static constexpr auto Byte = 8;

public:
    serializer_context(std::size_t size = 1)
        : m_data(std::min(std::size_t{ 1 }, size))
    {}

    /**
     * @brief   Serialize a value (Big-Endian).
     */
    template <typename T>
    void operator()(T value) {
        impl(value);
    }

    /**
     * @brief   Return a copy of the serialized data in a byte array.
     *
     * If the underlying vector is bigger than the serialized bytes, it will be
     * truncated.
     */
    [[nodiscard]] auto data() const -> bytes {
        auto ret = m_data;
        ret.resize(m_offset);
        return ret;
    }

private:
    bytes m_data;
    std::size_t m_offset = 0;

    template <std::unsigned_integral T>
    void impl(const T& x) {
        resize_if_needed(sizeof(T));

        for (std::size_t i = 0; i < sizeof(T); ++i) {
            const auto shift = (sizeof(T) - i - 1) * Byte;
            const auto mask = static_cast<T>(T{ 0xFF } << shift);

            m_data[m_offset] = std::byte{ static_cast<std::uint8_t>((x & mask) >> shift) };
            ++m_offset;
        }
    }

    void impl(std::string_view x) {
        copy_range(x);
    }

    void impl(const std::string& x) {
        copy_range(x);
    }

    template <typename T>
    void impl(const T& x) {
        serializer<T>{ }(*this, x);
    }

    template <typename Range>
        requires std::contiguous_iterator<typename Range::iterator>
            or std::is_array_v<Range>
    void copy_range(const Range& src) {
        resize_if_needed(src.size());

        using DT = bytes::difference_type;
        std::ranges::copy(data_view{ src }, std::next(std::begin(m_data), static_cast<DT>(m_offset)));
        m_offset += std::size(src);
    }

    void resize_if_needed(std::size_t size) {
        while (m_offset + size > m_data.size()) {
            m_data.resize(m_data.size() * 2);
        }
    }

};

/**
 * @brief   Serialize a type into a byte array.
 */
template <typename T>
[[nodiscard]] auto serialize(const T& x, std::size_t size = 1) -> bytes {
    auto ser = serializer_context{ size };
    ser(x);
    return ser.data();
}

/**
 * @brief   A stream buffer for binary data integrated with `data_view` (Big-Endian).
 *
 * Example code for writing into a socket.
 *
 * ```
 * auto buf = nova::stream_buffer{ 4096 };
 *
 * // Check `n` to see whether the complete data is written to buffer.
 * std::size_t n = buf.write("some binary data: \x00\x01\x02..."_data);
 *
 * while (not buf.empty()) {
 *     auto data = buf.view();
 *     auto n = send(socket, data.ptr(), data.size());
 *     buf.consume(n);
 * }
 * ```
 *
 * Six pointers are used to keep track of read and write area of the buffer.
 * - `eback`: Beginning of get area.
 * - `gptr`: Current byte of get area.
 * - `egptr`: End of get area.
 *
 * - `pbase`: Beginning of put area.
 * - `pptr`: Current byte of put area.
 * - `epptr`: End of put area.
 */
template <typename Allocator = std::allocator<std::byte>>
class stream_buffer : public std::basic_streambuf<std::byte> {
public:
    using streamsize = std::streamsize;
    using difference_type = std::vector<std::byte>::difference_type;

private:
    using base = std::basic_streambuf<std::byte>;
    static constexpr difference_type DefaultBufferDelta = 128;

public:

    /**
     * @brief   Allocates a buffer.
     *
     * Initial size is at most `buffer_delta`.
     *
     * Due to `std::basic_streambuf` API the maximum size is what an `int` can
     * represent.
     *
     * @throws  if the maximum size is above `int` limit.
     */
    stream_buffer(difference_type max_size, difference_type buffer_delta = DefaultBufferDelta)
        : m_max_size(max_size)
        , m_buffer_delta(buffer_delta)
    {
        static constexpr auto int_max = std::numeric_limits<int>::max();
        if (max_size > int_max) {
            throw nova::exception("Maximum buffer size ({}) is over the limit {}", max_size, int_max);
        }

        auto pend = std::min(max_size, m_buffer_delta);
        m_data.resize(static_cast<std::size_t>(pend));
        base::setg(m_data.data(), m_data.data(), m_data.data());
        base::setp(m_data.data(), std::next(m_data.data(), pend));
    }

    [[nodiscard]] auto size() const -> std::size_t {
        const auto d = std::distance(base::gptr(), base::pptr());
        return static_cast<std::size_t>(d);
    }

    [[nodiscard]] auto empty() const -> bool {
        return size() == 0;
    }

    /**
     * @brief   Data between `gptr` and `pptr`, i.e. avaliable but unread data.
     */
    [[nodiscard]] auto view() const -> data_view {
        return { std::next(m_data.data(), beg()), size() };
    }

    /**
     * @brief   Write data from view into the buffer.
     *
     * @returns with the number of bytes written into the buffer.
     */
    [[nodiscard]] auto write(data_view data) -> std::size_t {
        return static_cast<std::size_t>(
            base::sputn(data.ptr(), static_cast<streamsize>(data.size()))
        );
    }

    /**
     * @brief   Read some number of bytes.
     *
     * @returns with the number of successfully read bytes.
     *
     * Note: Not (yet) supported use case.
     */
    // [[nodiscard]] auto read(std::size_t n) -> std::size_t {
        // auto* ptr = ...;
        // const auto r = base::sgetn(ptr, static_cast<streamsize>(n));
        // return r;
    // }

    /**
     * @brief   Consume everything, effectively clearing the buffer.
     *
     * It will not resize the underlying buffer.
     */
    void consume() {
        consume(size());
    }

    /**
     * @brief   Consume some number of bytes from the buffer.
     */
    void consume(std::size_t n) {
        if (base::egptr() < base::pptr()) {
            setg(m_data.data(), base::gptr(), base::pptr());
        }

        if (std::next(base::gptr(), static_cast<int>(n)) > base::pptr()) {
            n = static_cast<std::size_t>(std::distance(base::gptr(), base::pptr()));
        }

        base::gbump(static_cast<int>(n));
    }

private:
    std::vector<std::byte> m_data;
    difference_type m_max_size;
    difference_type m_buffer_delta;

    /**
     * @brief   Get area calculated from the beginning of the buffer.
     */
    [[nodiscard]] auto beg() const -> difference_type {
        const auto* ptr = base::gptr();
        return std::distance(m_data.data(), ptr);
    }

    /**
     * @brief   Called when there is no more data to read.
     *
     * Note: Read operation is not (yet) supported.
     * Use `view()` to access the content of the buffer and
     * call `consume()` to discard the data that is no longer needed.
     */
    int_type underflow() override {
        if (base::gptr() < base::pptr()) {
            base::setg(m_data.data(), gptr(), pptr());
            return traits_type::to_int_type(*gptr());
        }
        return traits_type::eof();
    }

    /**
     * @brief   Called when the end of the write buffer is reached.
     *
     * @returns `eof` to indicate failure to ensure free write space.
     */
    int_type overflow(int_type c = traits_type::eof()) override {
        if (traits_type::eq_int_type(c, traits_type::eof())) {
            return traits_type::not_eof(c);
        }

        if (base::pptr() == base::epptr()) {
            difference_type n = [&]() {
                auto buffer_size = std::distance(base::gptr(), base::pptr());
                if (buffer_size < m_max_size && m_max_size - buffer_size < m_buffer_delta) {
                    return m_max_size - buffer_size;
                }
                return m_buffer_delta;
            }();

            if (not reserve(n)) {
                return traits_type::eof();
            }
        }

        *base::pptr() = traits_type::to_char_type(c);
        base::pbump(1);
        return c;
    }

    /**
     * @brief   Resize the underlying buffer if needed and if `max_size` is not reached.
     *
     * @returns false is the buffer is full.
     *
     * Shifts existing contents of get area to start of buffer.
     * Updates get and put pointers.
     *
     */
    auto reserve(difference_type n) -> bool {
        auto gnext = std::distance(m_data.data(), base::gptr());
        auto pnext = std::distance(m_data.data(), base::pptr());
        auto pend = std::distance(m_data.data(), base::epptr());

        if (n <= pend - pnext) {
            return true;
        }

        if (gnext > 0) {
            pnext -= gnext;
            std::memmove(
                m_data.data(),
                std::next(m_data.data(), gnext),
                static_cast<std::size_t>(pnext)
            );
        }

        if (n > pend - pnext) {
            if (n <= m_max_size && pnext <= m_max_size - n) {
                pend = pnext + n;
                m_data.resize(static_cast<std::size_t>(std::max<difference_type>(pend, 1)));
            }
            else {
                return false;
            }
        }

        setg(m_data.data(), m_data.data(), std::next(m_data.data(), pnext));
        setp(std::next(m_data.data(), pnext), std::next(m_data.data(), pend));

        return true;
    }

};

} // namespace nova

template <>
class fmt::formatter<nova::data_view> {
public:
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template <typename FmtContext>
    auto format(nova::data_view data, FmtContext& ctx) const {
        if (is_printable(data)) {
            return fmt::format_to(ctx.out(), "{}", data.as_string());
        }
        return fmt::format_to(ctx.out(), "x{}", data.as_hex_string());
    }

private:
    [[nodiscard]] static auto is_printable(nova::data_view data) -> bool {
        return not std::ranges::any_of(data, [](auto b) { return not nova::is_printable(b); });
    }

};
