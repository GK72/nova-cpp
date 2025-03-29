#pragma once

#include "nova/type_traits.hh"

#include <algorithm>
#include <array>
#include <cmath>
#include <cassert>
#include <cstdint>
#include <functional>
#include <numeric>
#include <ostream>
#include <type_traits>

namespace nova {

template <std::size_t Size, arithmetic Rep> class vec;

template <typename T>                       struct is_vec : std::false_type {};
template <std::size_t Size, arithmetic Rep> struct is_vec<vec<Size, Rep>> : std::true_type {};
template <std::size_t Size, arithmetic Rep> struct is_vec<const vec<Size, Rep>> : std::true_type {};
template <std::size_t Size, arithmetic Rep> struct is_vec<volatile vec<Size, Rep>> : std::true_type {};
template <std::size_t Size, arithmetic Rep> struct is_vec<const volatile vec<Size, Rep>> : std::true_type {};

template <typename T> constexpr bool is_vec_v = is_vec<T>::value;

template <typename T>
concept vec_like = requires {
    typename T::vec_type;
};

namespace detail {

    /**
     * @brief   Helper function object to iterate over the elements of the vector
     *
     * Vec-Vec operators and
     * Vec-factor operators
     * are supported
     */
    template <typename T, typename U>
    struct looper {
        looper(T& lhs_, U& rhs_)
            : lhs(lhs_)
            , rhs(rhs_)
        {}

        template <typename Func>
        constexpr T operator()(Func func) {
            auto result = T { };
            for (std::size_t i = 0; i < result.size(); ++i) {
                if constexpr (vec_like<U>) {
                    result[i] = func(lhs[i], rhs[i]);
                } else {
                    result[i] = func(lhs[i], rhs);
                }
            }
            return result;
        }

    private:
        T& lhs;
        U& rhs;
    };

} // namespace detail

template <std::size_t Size, arithmetic Rep>
class vec {
public:
    static_assert(Size > 0, "Empty vector is not valid!");

    using vec_type = vec<Size, Rep>;
    using rep = Rep;

    constexpr vec() = default;

    constexpr vec(std::array<Rep, Size> xs)
        : m_vec(std::move(xs))
    {}

    [[nodiscard]] constexpr std::size_t size() const noexcept { return Size; }

    [[nodiscard]] constexpr const Rep& operator[](std::size_t idx) const { return m_vec[idx]; }
    [[nodiscard]] constexpr       Rep& operator[](std::size_t idx)       { return m_vec[idx]; }

    [[nodiscard]] constexpr auto begin() const noexcept { return std::begin(m_vec); }
    [[nodiscard]] constexpr auto end()   const noexcept { return std::end(m_vec); }
    [[nodiscard]] constexpr auto begin()       noexcept { return std::begin(m_vec); }
    [[nodiscard]] constexpr auto end()         noexcept { return std::end(m_vec); }

    [[nodiscard]] Rep length() const noexcept {
        return norm();
    }

    /**
     * @brief   p-norm
     */
    [[nodiscard]] Rep norm(Rep p = 2) const noexcept {
        static_assert(floating_point<Rep>, "Cannot normalize a vector with non floating-point representation!");
        assert(p > 0);
        assert(not std::isnan(p));

        if (std::isinf(p)) {
            return *std::max_element(std::begin(*this), std::end(*this));
        }

        return std::pow(
            std::accumulate(
                std::begin(m_vec), std::end(m_vec),
                Rep { 0 },
                [&p](const Rep& init, const Rep& value) {
                    return init + std::pow(std::abs(value), p);
                }
            ),
            1 / p
        );
    }

    template <vec_like VecT>
    constexpr vec_type operator+=(const VecT& rhs) { *this = *this + rhs; return *this; }
    template <vec_like VecT>
    constexpr vec_type operator-=(const VecT& rhs) { *this = *this - rhs; return *this; }
    template <vec_like VecT>
    constexpr vec_type operator*=(const VecT& rhs) { *this = *this * rhs; return *this; }
    template <vec_like VecT>
    constexpr vec_type operator/=(const VecT& rhs) { *this = *this / rhs; return *this; }

    constexpr vec_type operator+=(const rep& rhs)  { *this = *this + rhs; return *this; }
    constexpr vec_type operator-=(const rep& rhs)  { *this = *this - rhs; return *this; }
    constexpr vec_type operator*=(const rep& rhs)  { *this = *this * rhs; return *this; }
    constexpr vec_type operator/=(const rep& rhs)  { *this = *this / rhs; return *this; }

protected:
    std::array<Rep, Size> m_vec;
};

template <vec_like VecT>
[[nodiscard]] std::ostream& operator<<(std::ostream& out, const VecT& value) {
    out << '{';
    for (std::size_t i = 0; i < value.size() - 1; ++i) {
        out << value[i] << ", ";
    }
    out << value[value.size() - 1] << "}";
    return out;
}

template <vec_like VecT, vec_like VecU>
[[nodiscard]] constexpr bool operator==(const VecT& lhs, const VecU& rhs) {
    for (std::size_t i = 0; i < lhs.size(); ++i) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
    }
    return true;
}

// ---------------------------------==[ Vec-Vec operators ]==---------------------------------------

template <vec_like VecT, vec_like VecU>
[[nodiscard]] constexpr bool operator!=(const VecT& lhs, const VecU& rhs) {
    return not (lhs == rhs);
}

template <vec_like VecT, vec_like VecU>
[[nodiscard]] constexpr VecT operator+(const VecT& lhs, const VecU& rhs) {
    return detail::looper(lhs, rhs)(std::plus<typename VecT::rep>{});

}

template <vec_like VecT, vec_like VecU>
[[nodiscard]] constexpr VecT operator-(const VecT& lhs, const VecU& rhs) {
    return detail::looper(lhs, rhs)(std::minus<typename VecT::rep>{});
}

template <vec_like VecT, vec_like VecU>
[[nodiscard]] constexpr VecT operator*(const VecT& lhs, const VecU& rhs) {
    return detail::looper(lhs, rhs)(std::multiplies<typename VecT::rep>{});
}

template <vec_like VecT, vec_like VecU>
[[nodiscard]] constexpr VecT operator/(const VecT& lhs, const VecU& rhs) {
    return detail::looper(lhs, rhs)(std::divides<typename VecT::rep>{});
}

template <vec_like VecT, vec_like VecU>
[[nodiscard]] constexpr typename VecT::rep dot(const VecT& lhs, const VecU& rhs) {
    return std::inner_product(
        std::begin(lhs),
        std::end(lhs),
        std::begin(rhs),
        typename VecT::rep { 0 }
    );
}

// --------------------------------==[ Vec-factor operators ]==-------------------------------------

template <vec_like VecT>
[[nodiscard]] constexpr VecT operator+(const VecT& lhs, const typename VecT::rep& rhs) {
    return detail::looper(lhs, rhs)(std::plus<typename VecT::rep>{});
}

template <vec_like VecT>
[[nodiscard]] constexpr VecT operator-(const VecT& lhs, const typename VecT::rep& rhs) {
    return detail::looper(lhs, rhs)(std::minus<typename VecT::rep>{});
}

template <vec_like VecT>
[[nodiscard]] constexpr VecT operator*(const VecT& lhs, const typename VecT::rep& rhs) {
    return detail::looper(lhs, rhs)(std::multiplies<typename VecT::rep>{});
}

template <vec_like VecT>
[[nodiscard]] constexpr VecT operator/(const VecT& lhs, const typename VecT::rep& rhs) {
    return detail::looper(lhs, rhs)(std::divides<typename VecT::rep>{});
}

template <vec_like VecT>
[[nodiscard]] constexpr typename VecT::rep product(const VecT& value) noexcept {
    const auto& vec = static_cast<typename VecT::vec_type>(value);
    return std::accumulate(
        std::begin(vec),
        std::end(vec),
        typename VecT::rep { 1 },
        std::multiplies{}
    );
}

template <vec_like VecT>
[[nodiscard]] constexpr VecT unit(const VecT& value) noexcept {
    return value / value.length();
}

// ---------------------------------==[ "Famous" dimensions ]==-------------------------------------

template <typename Rep>
class vec2 : public vec<2, Rep> {
public:
    using vec_type = vec<2, Rep>;

    constexpr vec2() = default;

    constexpr vec2(Rep x, Rep y)
        : vec_type(std::to_array({ x, y }))
    {}

    [[nodiscard]] constexpr Rep x() const noexcept { return vec_type::m_vec[0]; }
    [[nodiscard]] constexpr Rep y() const noexcept { return vec_type::m_vec[1]; }
    [[nodiscard]]           Rep& x()      noexcept { return vec_type::m_vec[0]; }
    [[nodiscard]]           Rep& y()      noexcept { return vec_type::m_vec[1]; }

// Extension point, e.g. for ImGui Vec conversions
#ifdef NOVA_EXTENSION_CLASS_VEC2
    NOVA_EXTENSION_CLASS_VEC2
#endif
};

template <typename Rep>
[[nodiscard]] constexpr Rep area(const vec2<Rep>& value) noexcept {
    return product(value);
}

template <typename Rep>
class vec3 : public vec<3, Rep> {
public:
    using vec_type = vec<3, Rep>;

    constexpr vec3() = default;

    constexpr vec3(Rep x, Rep y, Rep z)
        : vec_type(std::to_array({ x, y, z }))
    {}

    [[nodiscard]] constexpr Rep x() const noexcept { return vec_type::m_vec[0]; }
    [[nodiscard]] constexpr Rep y() const noexcept { return vec_type::m_vec[1]; }
    [[nodiscard]] constexpr Rep z() const noexcept { return vec_type::m_vec[2]; }
    [[nodiscard]]           Rep& x()      noexcept { return vec_type::m_vec[0]; }
    [[nodiscard]]           Rep& y()      noexcept { return vec_type::m_vec[1]; }
    [[nodiscard]]           Rep& z()      noexcept { return vec_type::m_vec[2]; }

// Extension point, e.g. for ImGui Vec conversions
#ifdef NOVA_EXTENSION_CLASS_VEC3
    NOVA_EXTENSION_CLASS_VEC3
#endif
};

template <typename Rep>
[[nodiscard]] constexpr Rep volume(const vec3<Rep>& value) noexcept {
    return product(value);
}

/**
 * @brief   Cross product
 *
 * Given two linearly independent vector `lhs` and `rhs`, the cross product
 * is a vector perpendicular to both `lhs` and `rhs`.
 *
 * TODO(doc): latex doc for definiton
 */
template <typename Rep>
[[nodiscard]] constexpr vec3<Rep> cross(const vec3<Rep>& lhs, const vec3<Rep>& rhs) {
    return {
        lhs.y() * rhs.z() - lhs.z() * rhs.y(),
        lhs.z() * rhs.x() - lhs.x() * rhs.z(),
        lhs.x() * rhs.y() - lhs.y() * rhs.x()
    };
}

template <typename Rep>
class vec4 : public vec<4, Rep> {
public:
    using vec_type = vec<4, Rep>;

    constexpr vec4() = default;

    constexpr vec4(Rep x, Rep y, Rep z, Rep w)
        : vec_type(std::to_array({ x, y, z, w }))
    {}

    [[nodiscard]] constexpr Rep x() const noexcept { return vec_type::m_vec[0]; }
    [[nodiscard]] constexpr Rep y() const noexcept { return vec_type::m_vec[1]; }
    [[nodiscard]] constexpr Rep z() const noexcept { return vec_type::m_vec[2]; }
    [[nodiscard]] constexpr Rep w() const noexcept { return vec_type::m_vec[3]; }
    [[nodiscard]]           Rep& x()      noexcept { return vec_type::m_vec[0]; }
    [[nodiscard]]           Rep& y()      noexcept { return vec_type::m_vec[1]; }
    [[nodiscard]]           Rep& z()      noexcept { return vec_type::m_vec[2]; }
    [[nodiscard]]           Rep& w()      noexcept { return vec_type::m_vec[3]; }

// Extension point, e.g. for ImGui Vec conversions
#ifdef NOVA_EXTENSION_CLASS_VEC4
    NOVA_EXTENSION_CLASS_VEC4
#endif
};

// ---------------------------------==[ Utility functions ]==---------------------------------------

/**
 * @brief   Clamp between 0 and 255 for packing four 8-bit values
 */
[[nodiscard]] constexpr auto cast8(arithmetic auto x) noexcept {
    return static_cast<std::uint8_t>(std::clamp(x, 0, 255));
}

/**
 * @brief   Clamp between 0 and 255 for packing four 8-bit values
 */
[[nodiscard]] constexpr auto cast8(float x) noexcept {
    return static_cast<std::uint8_t>(std::clamp(x, 0.0F, 1.0F) * 255.0F);
}

/**
 * @brief   Pack 8-bit values into one 32 bit value (Big endian)
 *
 * Note: no little endian pair, use parameters in reverse order
 */
template <arithmetic T>
[[nodiscard]] constexpr std::uint32_t pack32BE(T x, T y, T z, T w) noexcept {
    return static_cast<std::uint32_t>(cast8(x) << 24)
         | static_cast<std::uint32_t>(cast8(y) << 16)
         | static_cast<std::uint32_t>(cast8(z) << 8)
         | cast8(w);
}

/**
 * @brief   Pack a 4D vector as 8-bit values into one 32 bit value (Big endian)
 */
template <typename Rep>
[[nodiscard]] constexpr std::uint32_t pack32BE(const vec4<Rep>& value) noexcept {
    return pack32BE(value.x(), value.y(), value.z(), value.w());
}

/**
 * @brief   Pack 8-bit values into one 32 bit value (Little endian)
 */
template <typename Rep>
[[nodiscard]] constexpr std::uint32_t pack32LE(const vec4<Rep>& value) noexcept {
    return pack32BE(value.w(), value.z(), value.y(), value.x());
}

using Vec2i = vec2<int>;
using Vec3i = vec3<int>;
using Vec4i = vec4<int>;

using Vec2f = vec2<float>;
using Vec3f = vec3<float>;
using Vec4f = vec4<float>;

using Vec2d = vec2<double>;
using Vec3d = vec3<double>;
using Vec4d = vec4<double>;

} // namespace nova
