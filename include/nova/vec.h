#pragma once

#include <array>
#include <functional>
#include <numeric>
#include <type_traits>

namespace nova {

template <std::size_t Dim, typename Rep> class vec;

template <typename T>                    struct is_vec : std::false_type {};
template <std::size_t Dim, typename Rep> struct is_vec<vec<Dim, Rep>> : std::true_type {};
template <std::size_t Dim, typename Rep> struct is_vec<const vec<Dim, Rep>> : std::true_type {};
template <std::size_t Dim, typename Rep> struct is_vec<volatile vec<Dim, Rep>> : std::true_type {};
template <std::size_t Dim, typename Rep> struct is_vec<const volatile vec<Dim, Rep>> : std::true_type {};

template <typename T> constexpr bool is_vec_v = is_vec<T>::value;

template <typename T>
concept vec_like = requires {
    typename T::vec_type;
};


template <std::size_t Dim, typename Rep>
class vec {
public:
    using vec_type = vec<Dim, Rep>;
    using rep = Rep;

    constexpr vec() = default;

    constexpr vec(std::array<Rep, Dim> xs)
        : m_vec(std::move(xs))
    {}

    [[nodiscard]] constexpr std::size_t dimensions() const noexcept { return Dim; }

    [[nodiscard]] constexpr const Rep& operator[](std::size_t idx) const { return m_vec[idx]; }
    [[nodiscard]] constexpr       Rep& operator[](std::size_t idx)       { return m_vec[idx]; }

    [[nodiscard]] constexpr auto begin() const noexcept { return std::begin(m_vec); }
    [[nodiscard]] constexpr auto end()   const noexcept { return std::end(m_vec); }
    [[nodiscard]] constexpr auto begin()       noexcept { return std::begin(m_vec); }
    [[nodiscard]] constexpr auto end()         noexcept { return std::end(m_vec); }

    [[nodiscard]] constexpr Rep length_squared() const noexcept {
        return std::inner_product(
            std::begin(m_vec), std::end(m_vec),
            std::begin(m_vec),
            Rep { 0 },
            std::plus<Rep>{}, std::multiplies<Rep>{}
        );
    }

    [[nodiscard]] constexpr Rep length() const noexcept {
        return std::sqrt(length_squared());
    }

    template <vec_like VecT>
    constexpr vec_type operator+=(const VecT& rhs) { *this = *this + rhs; return *this; }
    template <vec_like VecT>
    constexpr vec_type operator-=(const VecT& rhs) { *this = *this - rhs; return *this; }
    template <vec_like VecT>
    constexpr vec_type operator*=(const VecT& rhs) { *this = *this * rhs; return *this; }
    template <vec_like vect>
    constexpr vec_type operator/=(const vect& rhs) { *this = *this / rhs; return *this; }

    constexpr vec_type operator+=(const rep& rhs)  { *this = *this + rhs; return *this; }
    constexpr vec_type operator-=(const rep& rhs)  { *this = *this - rhs; return *this; }
    constexpr vec_type operator*=(const rep& rhs)  { *this = *this * rhs; return *this; }
    constexpr vec_type operator/=(const rep& rhs)  { *this = *this / rhs; return *this; }

protected:
    std::array<Rep, Dim> m_vec;
};

template <vec_like VecT>
[[nodiscard]] std::ostream& operator<<(std::ostream& out, const VecT& value) {
    out << '{';
    for (std::size_t i = 0; i < value.dimensions() - 1; ++i) {
        out << value[i] << ", ";
    }
    out << value[value.dimensions() - 1] << "}";
    return out;
}

template <vec_like VecT, vec_like VecU>
[[nodiscard]] constexpr bool operator==(const VecT& lhs, const VecU& rhs) {
    for (std::size_t i = 0; i < lhs.dimensions(); ++i) {
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
    auto ret = VecT { };
    for (std::size_t i = 0; i < lhs.dimensions(); ++i) {
        ret[i] = lhs[i] + rhs[i];
    }
    return ret;
}

template <vec_like VecT, vec_like VecU>
[[nodiscard]] constexpr VecT operator-(const VecT& lhs, const VecU& rhs) {
    auto ret = VecT { };
    for (std::size_t i = 0; i < lhs.dimensions(); ++i) {
        ret[i] = lhs[i] - rhs[i];
    }
    return ret;
}

template <vec_like VecT, vec_like VecU>
[[nodiscard]] constexpr VecT operator*(const VecT& lhs, const VecU& rhs) {
    auto ret = VecT { };
    for (std::size_t i = 0; i < lhs.dimensions(); ++i) {
        ret[i] = lhs[i] * rhs[i];
    }
    return ret;
}

template <vec_like VecT, vec_like VecU>
[[nodiscard]] constexpr VecT operator/(const VecT& lhs, const VecU& rhs) {
    auto ret = VecT { };
    for (std::size_t i = 0; i < lhs.dimensions(); ++i) {
        ret[i] = lhs[i] / rhs[i];
    }
    return ret;
}

template <vec_like VecT, vec_like VecU>
[[nodiscard]] constexpr VecT::rep dot(const VecT& lhs, const VecU& rhs) {
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
    auto ret = VecT { };
    for (std::size_t i = 0; i < lhs.dimensions(); ++i) {
        ret[i] = lhs[i] + rhs;
    }
    return ret;
}

template <vec_like VecT>
[[nodiscard]] constexpr VecT operator-(const VecT& lhs, const typename VecT::rep& rhs) {
    auto ret = VecT { };
    for (std::size_t i = 0; i < lhs.dimensions(); ++i) {
        ret[i] = lhs[i] - rhs;
    }
    return ret;
}

template <vec_like VecT>
[[nodiscard]] constexpr VecT operator*(const VecT& lhs, const typename VecT::rep& rhs) {
    auto ret = VecT { };
    for (std::size_t i = 0; i < lhs.dimensions(); ++i) {
        ret[i] = lhs[i] * rhs;
    }
    return ret;
}

template <vec_like VecT>
[[nodiscard]] constexpr VecT operator/(const VecT& lhs, const typename VecT::rep& rhs) {
    auto ret = VecT { };
    for (std::size_t i = 0; i < lhs.dimensions(); ++i) {
        ret[i] = lhs[i] / rhs;
    }
    return ret;
}






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
};

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
};

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

} // namespace nova
