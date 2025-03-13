/**
 * Part of Nova C++ Library.
 *
 * Handling binary data; serialization and deserialization.
 *
 * - Deserialization: `data_view` for safely interpreting binary data.
 * - Serialization:
 *   - `serializer_context` class for low-level handling
 *   - `serialize(x)` free function for convenience
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

#include "nova/error.hh"
#include "nova/type_traits.hh"

#include <fmt/core.h>

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <span>
#include <streambuf>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

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
        : m_data(reinterpret_cast<const std::byte*>(ptr), size)
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
    [[nodiscard]] auto char_ptr() const -> const char*      { return reinterpret_cast<const char*>(m_data.data()); }

    /**
     * @brief   Interpret data as number according to the type `T`.
     */
    template <typename T = std::size_t>
        requires std::is_integral_v<T>
    [[nodiscard]] auto as_number(std::size_t pos, std::size_t length) const -> T {
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
     * @brief   Interpret data as number according to the type `T`.
     */
    template <typename T>
        requires std::is_integral_v<T>
    [[nodiscard]] auto as_number(std::size_t pos) const -> T {
        return as_number<T>(pos, sizeof(T));
    }

    /**
     * @brief   Interpret data for the given `length` as a string
     */
    [[nodiscard]]
    auto as_string(std::size_t pos, std::size_t length) const -> std::string_view {
        boundary_check(pos, length);
        return { reinterpret_cast<const char*>(m_data.data() + pos), length };
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
                throw exception("Out of bounds access: {}", detail::data_cursor{ pos, length, size() });
            }
        }
        else {
            nova_assert(pos + length <= size());
        }
    }
};

} // namespace detail

using data_view = detail::data_view<>;
using data_view_be = detail::data_view<endian::big>;
using data_view_le = detail::data_view<endian::little>;

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

// TODO(feat): Allocator-aware buffer.
class stream_buffer : public std::basic_streambuf<std::byte> {
    using Base = std::basic_streambuf<std::byte>;
    using difference_type = std::vector<std::byte>::difference_type;

    static constexpr auto BufferDelta = 128;

public:
    stream_buffer(long max_size)
        : m_stream(this)
        , m_max_size(max_size)
    {
        long pend = BufferDelta;
        m_data.reserve(BufferDelta);
        Base::setg(m_data.data(), m_data.data(), m_data.data());
        Base::setp(m_data.data(), std::next(m_data.data(), pend));
    }

    [[nodiscard]] auto size() const -> std::size_t {
        const auto d = std::distance(Base::gptr(), Base::pptr());
        nova_assert(d >= 0);    // TODO: Prove that this is always true.
        return static_cast<std::size_t>(d);
    }

    [[nodiscard]] auto view() const -> data_view {
        return { std::next(m_data.data(), beg()), size() };
    }

    auto write(const std::byte* ptr, long n) {
        m_stream.write(ptr, n);
    }

    auto consume(difference_type n) {
        if (Base::egptr() < Base::pptr()) {
            setg(m_data.data(), Base::gptr(), Base::pptr());
        }

        if (std::next(Base::gptr(), n) > Base::pptr()) {
            n = std::distance(Base::gptr(), Base::pptr());
        }

        Base::gbump(static_cast<int>(n));
    }

private:
    std::vector<std::byte> m_data;
    std::basic_ostream<std::byte> m_stream;
    long m_max_size;

    [[nodiscard]] auto beg() const -> difference_type {
        const auto* ptr = Base::gptr();
        return std::distance(m_data.data(), ptr);
    }

    int_type underflow() override {
        if (Base::gptr() < Base::pptr()) {
            Base::setg(m_data.data(), gptr(), pptr());
            return traits_type::to_int_type(*gptr());
        }
        return traits_type::eof();
    }

    int_type overflow(int_type c = traits_type::eof()) override {
        if (traits_type::eq_int_type(c, traits_type::eof())) {
            return traits_type::not_eof(c);
        }

        if (Base::pptr() == Base::epptr()) {
            auto buffer_size = std::distance(Base::gptr(), Base::pptr());
            if (buffer_size < m_max_size && m_max_size - buffer_size < BufferDelta) {
                reserve(m_max_size - buffer_size);
            }
            else {
                reserve(BufferDelta);
            }
        }

        *Base::pptr() = traits_type::to_char_type(c);
        Base::pbump(1);
        return c;
    }

    void reserve(long n) {
        // Get current stream positions as offsets.
        auto gnext = std::distance(m_data.data(), Base::gptr());
        auto pnext = std::distance(m_data.data(), Base::pptr());
        auto pend = std::distance(m_data.data(), Base::epptr());

        // Check if there is already enough space in the put area.
        if (n <= pend - pnext) {
            return;
        }

        // Shift existing contents of get area to start of buffer.
        if (gnext > 0) {
            pnext -= gnext;
            std::memmove(
                m_data.data(),
                std::next(m_data.data(), gnext),
                static_cast<std::size_t>(pnext)
            );
        }

        // Ensure buffer is large enough to hold at least the specified size.
        if (n > pend - pnext) {
            if (n <= m_max_size && pnext <= m_max_size - n) {
                pend = pnext + n;
                m_data.resize(static_cast<std::size_t>(std::max<long>(pend, 1)));
            }
            else {
                // TODO(err): Avoid exceptions.
                throw exception("Streambuf too long");
            }
        }

        // Update stream positions.
        setg(m_data.data(), m_data.data(), std::next(m_data.data(), pnext));
        setp(std::next(m_data.data(), pnext), std::next(m_data.data(), pend));
    }

};

} // namespace nova
