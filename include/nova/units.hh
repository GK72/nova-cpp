#pragma once

#include <chrono>
#include <numeric>
#include <ratio>
#include <type_traits>

namespace nova::units {

template <typename Unit, typename Rep, typename Ratio = std::ratio<1>> class measure;

// -----------------------------------==[ Tag dispatches ]==----------------------------------------

struct rates {};

struct duration {};

struct length;
struct speed : rates { using base = length; };
struct length { using rate = speed; };

struct data_volume;
struct data_rate : rates { using base = data_volume; };
struct data_volume { using rate = data_rate; };

namespace detail {

    template <typename R1, typename R2>
    struct ratioGCD
    {
        using type =
            std::ratio<std::gcd(R1::num, R2::num),
                       std::lcm(R1::den, R2::den)>;
    };

    struct division_tag {};
    struct multiplication_tag {};

} // namespace detail
} // namespace nova::units

// -----------------------------==[ Common_Type Specialization ]==----------------------------------

namespace std {

    /**
     * @brief   General specialization for common types
     */
    template <typename Unit1, typename Unit2,
              typename Rep1, typename Ratio1,
              typename Rep2, typename Ratio2
    >
    struct common_type<
        nova::units::measure<Unit1, Rep1, Ratio1>,
        nova::units::measure<Unit2, Rep2, Ratio2>>
    {
        using type = nova::units::measure<
            typename common_type<Unit1, Unit2>::type,
            typename common_type<Rep1, Rep2>::type,
            typename nova::units::detail::ratioGCD<Ratio1, Ratio2>::type>;
    };

    /**
     * @brief   Specialization for creating rate
     */
    template <typename Unit1, typename Unit2,
              typename Rep1, typename Ratio1,
              typename Rep2, typename Ratio2
    >
    struct common_type<
        nova::units::measure<Unit1, Rep1, Ratio1>,
        nova::units::measure<Unit2, Rep2, Ratio2>,
        nova::units::detail::division_tag
    >
    {
        using type = nova::units::measure<
            typename Unit1::rate,
            typename common_type<Rep1, Rep2>::type,
            typename nova::units::detail::ratioGCD<Ratio1, Ratio2>::type>;
    };

    /**
     * @brief   Specialization for converting rate back to its base
     */
    template <typename Unit1, typename Unit2,
              typename Rep1, typename Ratio1,
              typename Rep2, typename Ratio2
    >
        requires std::is_base_of_v<nova::units::rates, Unit1>
    struct common_type<
        nova::units::measure<Unit1, Rep1, Ratio1>,
        nova::units::measure<Unit2, Rep2, Ratio2>,
        nova::units::detail::multiplication_tag
    >
    {
        using type = nova::units::measure<
            typename Unit1::base,
            typename common_type<Rep1, Rep2>::type,
            typename nova::units::detail::ratioGCD<Ratio1, Ratio2>::type>;
    };

} // namespace std

namespace nova::units {

// -------------------------------------==[ Type Trait ]==------------------------------------------

template <typename T>
struct is_measure : std::false_type {};

template <typename Unit, typename Rep, typename Ratio>
struct is_measure<measure<Unit, Rep, Ratio>> : std::true_type {};

template <typename Unit, typename Rep, typename Ratio>
struct is_measure<const measure<Unit, Rep, Ratio>> : std::true_type {};

template <typename Unit, typename Rep, typename Ratio>
struct is_measure<volatile measure<Unit, Rep, Ratio>> : std::true_type {};

template <typename Unit, typename Rep, typename Ratio>
struct is_measure<const volatile measure<Unit, Rep, Ratio>> : std::true_type {};

template <typename T>
constexpr bool is_measure_v = is_measure<T>::value;

template <typename T>
struct is_ratio : std::false_type {};

template <intmax_t N, intmax_t D>
struct is_ratio<std::ratio<N, D>> : std::true_type {};

template <typename T>
constexpr bool is_ratio_v = is_ratio<T>::value;

// --------------------------------------==[ Casting ]==--------------------------------------------

namespace detail {

    /**
     * @brief No conversion
     */
    template <typename Frommeasure, typename Tomeasure,
              typename Ratio = typename std::ratio_divide<
                    typename Frommeasure::Ratio,
                    typename Tomeasure::Ratio>::type,
              bool = Ratio::num == 1,
              bool = Ratio::den == 1>
    struct measure_cast;

    /**
     * @brief Representation casting
     */
    template <typename Frommeasure, typename Tomeasure, typename Ratio>
    struct measure_cast<Frommeasure, Tomeasure, Ratio, /* num = 1 */ true, /* den = 1 */ true>
    {
        constexpr Tomeasure operator()(const Frommeasure& x) const noexcept {
            return Tomeasure(static_cast<typename Tomeasure::Rep>(x.count()));
        }
    };

    /**
     * @brief Representation casting and Ratio down conversion
     */
    template <typename Frommeasure, typename Tomeasure, typename Ratio>
    struct measure_cast<Frommeasure, Tomeasure, Ratio, /* num = 1 */ true, /* den = 1 */ false>
    {
        constexpr Tomeasure operator()(const Frommeasure& x) const noexcept {
            using CT =
                typename std::common_type_t<
                    typename Tomeasure::Rep,
                    typename Frommeasure::Rep,
                    intmax_t>;
            return Tomeasure(static_cast<typename Tomeasure::Rep>(
                static_cast<CT>(x.count()) / static_cast<CT>(Ratio::den))
            );
        }
    };

    /**
     * @brief Representation casting and Ratio up conversion
     */
    template <typename Frommeasure, typename Tomeasure, typename Ratio>
    struct measure_cast<Frommeasure, Tomeasure, Ratio, /* num = 1 */ false, /* den = 1 */ true>
    {
        constexpr Tomeasure operator()(const Frommeasure& x) const noexcept {
            using CT =
                typename std::common_type_t<
                    typename Tomeasure::Rep,
                    typename Frommeasure::Rep,
                    intmax_t>;
            return Tomeasure(static_cast<typename Tomeasure::Rep>(
                static_cast<CT>(x.count()) * static_cast<CT>(Ratio::num))
            );
        }
    };

    /**
     * @brief Representation casting and Ratio conversion
     */
    template <typename Frommeasure, typename Tomeasure, typename Ratio>
    struct measure_cast<Frommeasure, Tomeasure, Ratio, /* num = 1 */ false, /* den = 1 */ false>
    {
        constexpr Tomeasure operator()(const Frommeasure& x) const noexcept {
            using CT =
                typename std::common_type_t<
                    typename Tomeasure::Rep,
                    typename Frommeasure::Rep,
                    intmax_t>;;
            return Tomeasure(static_cast<typename Tomeasure::Rep>(
                static_cast<CT>(x.count())
                    * static_cast<CT>(Ratio::num)
                    / static_cast<CT>(Ratio::den))
            );
        }
    };

    // ----------------------==[ Relational operator implementations ]==----------------------------

    template <typename Lhs, typename Rhs>             // ----==[ Different types ]==----
    struct measure_eq {
        [[nodiscard]] constexpr
        bool operator()(const Lhs& lhs, const Rhs& rhs) const noexcept {
            using CT = typename std::common_type_t<Lhs, Rhs>;
            return CT(lhs).count() == CT(rhs).count();
        }
    };

    template <typename Lhs, typename Rhs>
    struct measure_lt {
        [[nodiscard]] constexpr
        bool operator()(const Lhs& lhs, const Rhs& rhs) const noexcept {
            using CT = typename std::common_type_t<Lhs, Rhs>;
            return CT(lhs).count() < CT(rhs).count();
        }
    };

    template <typename Lhs>                        // ----==[ Common type ]==----
    struct measure_eq<Lhs, Lhs> {
        [[nodiscard]] constexpr
        bool operator()(const Lhs& lhs, const Lhs& rhs) const noexcept {
            return lhs.count() == rhs.count();
        }
    };

    template <typename Lhs>
    struct measure_lt<Lhs, Lhs> {
        [[nodiscard]] constexpr
        bool operator()(const Lhs& lhs, const Lhs& rhs) const noexcept {
            return lhs.count() < rhs.count();
        }
    };

    template <typename Rep>
    struct measure_values {
        static constexpr Rep zero() noexcept { return Rep(0); }
        static constexpr Rep min()  noexcept { return std::numeric_limits<Rep>::lowest(); }
        static constexpr Rep max()  noexcept { return std::numeric_limits<Rep>::max(); }
    };

} // namespace detail

/**
 * @brief Wrapper for casting
 */
template <typename Tomeasure, typename Unit, typename Rep, typename Ratio>
[[nodiscard]] constexpr
typename std::enable_if_t<is_measure_v<Tomeasure>, Tomeasure>
measure_cast(const measure<Unit, Rep, Ratio>& x) noexcept
{
    return detail::measure_cast<measure<Unit, Rep, Ratio>, Tomeasure>()(x);
}

// ----------------------------------==[ measure CLASS ]==--------------------------------------

template <typename UnitT, typename RepT, typename RatioT>
class measure {
    static_assert(!is_measure_v<RepT>, "A measure representation can not be a measure");
    static_assert(is_ratio_v<RatioT>, "Second template parameter must be a std::ratio");
    static_assert(RatioT::num > 0, "Ratio must be positive");

public:
    using Ratio    = typename RatioT::type;
    using Rep      = RepT;
    using Unit = UnitT;

    constexpr measure() = default;

    template <typename Rep2>
    constexpr explicit measure(const Rep2& value,
        typename std::enable_if_t<
            std::is_convertible_v<Rep2, Rep>
            && (std::is_floating_point_v<Rep>
            || !std::is_floating_point_v<Rep2>)
        >* = 0      // NOLINT(readability-named-parameter)
    )
        : m_value(value)
    {}

    template <typename Unit2, typename Rep2, typename Ratio2>
    constexpr measure(const measure<Unit2, Rep2, Ratio2>& value,
        typename std::enable_if_t<
            std::is_floating_point_v<Rep>
            || (std::ratio_divide<Ratio2, Ratio>::den == 1
            && !std::is_floating_point_v<Rep2>)
        >* = 0      // NOLINT(readability-named-parameter)
    )
        : m_value(measure_cast<measure>(value).count())
    {}

    constexpr Rep count() const { return m_value; }

    typename std::common_type_t<measure> operator+() const {
        return typename std::common_type_t<measure>(*this);
    }

    typename std::common_type_t<measure> operator-() const {
        return typename std::common_type_t<measure>(-m_value);
    }

    constexpr measure  operator++(int) { return measure(m_value++);  }
    constexpr measure  operator--(int) { return measure(m_value--);  }
    constexpr measure& operator++()    { ++m_value; return *this; }
    constexpr measure& operator--()    { --m_value; return *this; }

    constexpr measure& operator+=(const measure& rhs) { m_value += rhs.count(); return *this; }
    constexpr measure& operator-=(const measure& rhs) { m_value -= rhs.count(); return *this; }
    constexpr measure& operator*=(const Rep& rhs)  { m_value *= rhs;         return *this; }
    constexpr measure& operator/=(const Rep& rhs)  { m_value /= rhs;         return *this; }
    constexpr measure& operator%=(const Rep& rhs)  { m_value %= rhs;         return *this; }
    constexpr measure& operator%=(const measure& rhs) { m_value += rhs.count(); return *this; }

    static constexpr measure zero() noexcept { return measure(detail::measure_values<Rep>::zero()); }
    static constexpr measure min()  noexcept { return measure(detail::measure_values<Rep>::min()); }
    static constexpr measure max()  noexcept { return measure(detail::measure_values<Rep>::max()); }

private:
    Rep m_value;

};

/**
 * @brief   Adapter for `std::chrono::duration`
 */
template <typename RepT, typename RatioT>
class measure<duration, RepT, RatioT> : public std::chrono::duration<RepT, RatioT> {
public:
    using Ratio    = typename RatioT::type;
    using Rep      = RepT;

    constexpr explicit measure(const std::chrono::duration<RepT, RatioT>& value)
        : std::chrono::duration<RepT, RatioT>(value)
    {}

    constexpr explicit measure(RepT value)
        : std::chrono::duration<RepT, RatioT>(value)
    {}
};

template <typename RepT, typename RatioT>
measure(std::chrono::duration<RepT, RatioT> a) -> measure<duration, RepT, RatioT>;

/**
 * @brief   Adapter for division by `std::chrono::duration`
 */
template <typename Unit1, typename Rep1, typename Ratio1, typename Rep2, typename Ratio2>
[[nodiscard]] constexpr auto
operator/(const measure<Unit1, Rep1, Ratio1>& lhs, const std::chrono::duration<Rep2, Ratio2>& rhs) noexcept {
    return lhs / measure{ rhs };
}

/**
 * @brief   Adapter for multiplication by `std::chrono::duration`
 */
template <typename Unit1, typename Rep1, typename Ratio1, typename Rep2, typename Ratio2>
[[nodiscard]] constexpr auto
operator*(const measure<Unit1, Rep1, Ratio1>& lhs, const std::chrono::duration<Rep2, Ratio2>& rhs) noexcept {
    return lhs * measure{ rhs };
}

// --------------------------------==[ Relational operators ]==-------------------------------------

template <typename Unit1, typename Rep1, typename Ratio1, typename Unit2, typename Rep2, typename Ratio2>
[[nodiscard]] constexpr
bool operator==(const measure<Unit1, Rep1, Ratio1>& lhs, const measure<Unit2, Rep2, Ratio2>& rhs) noexcept {
    return detail::measure_eq<measure<Unit1, Rep1, Ratio1>, measure<Unit2, Rep2, Ratio2>>()(lhs, rhs);
};

template <typename Unit1, typename Rep1, typename Ratio1, typename Unit2, typename Rep2, typename Ratio2>
[[nodiscard]] constexpr
bool operator!=(const measure<Unit1, Rep1, Ratio1>& lhs, const measure<Unit2, Rep2, Ratio2>& rhs) noexcept {
    return !(lhs == rhs);
};

template <typename Unit1, typename Rep1, typename Ratio1, typename Unit2, typename Rep2, typename Ratio2>
[[nodiscard]] constexpr
bool operator<(const measure<Unit1, Rep1, Ratio1>& lhs, const measure<Unit2, Rep2, Ratio2>& rhs) noexcept {
    return detail::measure_lt<measure<Unit1, Rep1, Ratio1>, measure<Unit2, Rep2, Ratio2>>()(lhs, rhs);
};

template <typename Unit1, typename Rep1, typename Ratio1, typename Unit2, typename Rep2, typename Ratio2>
[[nodiscard]] constexpr
bool operator>(const measure<Unit1, Rep1, Ratio1>& lhs, const measure<Unit2, Rep2, Ratio2>& rhs) noexcept {
    return rhs < lhs;
};

template <typename Unit1, typename Rep1, typename Ratio1, typename Unit2, typename Rep2, typename Ratio2>
[[nodiscard]] constexpr
bool operator>=(const measure<Unit1, Rep1, Ratio1>& lhs, const measure<Unit2, Rep2, Ratio2>& rhs) noexcept {
    return !(lhs < rhs);
};

template <typename Unit1, typename Rep1, typename Ratio1, typename Unit2, typename Rep2, typename Ratio2>
[[nodiscard]] constexpr
bool operator<=(const measure<Unit1, Rep1, Ratio1>& lhs, const measure<Unit2, Rep2, Ratio2>& rhs) noexcept {
    return !(lhs > rhs);
};

// --------------------------------==[ Arithmetic operators ]==-------------------------------------

template <typename Unit1, typename Rep1, typename Ratio1, typename Unit2, typename Rep2, typename Ratio2>
[[nodiscard]] constexpr
typename std::common_type_t<measure<Unit1, Rep1, Ratio1>, measure<Unit2, Rep2, Ratio2>>
operator+(const measure<Unit1, Rep1, Ratio1>& lhs, const measure<Unit2, Rep2, Ratio2>& rhs) noexcept
{
    using CT =
        typename std::common_type_t<
            measure<Unit1, Rep1, Ratio1>,
            measure<Unit2, Rep2, Ratio2>
        >;

    return CT(
        CT(lhs).count() + CT(rhs).count()
    );
}

template <typename Unit1, typename Rep1, typename Ratio1, typename Unit2, typename Rep2, typename Ratio2>
[[nodiscard]] constexpr
typename std::common_type_t<measure<Unit1, Rep1, Ratio1>, measure<Unit2, Rep2, Ratio2>>
operator-(const measure<Unit1, Rep1, Ratio1>& lhs, const measure<Unit2, Rep2, Ratio2>& rhs) noexcept
{
    using CT =
        typename std::common_type_t<
            measure<Unit1, Rep1, Ratio1>,
            measure<Unit2, Rep2, Ratio2>
        >;

    return CT(
        CT(lhs).count() - CT(rhs).count()
    );
}

/**
 * @brief   Multiplication by scalar
 */
template <typename Unit, typename Rep1, typename Ratio, typename Rep2>
[[nodiscard]] constexpr
typename std::enable_if<
    std::is_convertible_v<Rep2, typename std::common_type_t<Rep1, Rep2>>,
    measure<Unit, typename std::common_type_t<Rep1, Rep2>, Ratio>
>::type
operator*(const measure<Unit, Rep1, Ratio>& lhs, const Rep2& rhs) noexcept
{
    using CR = typename std::common_type_t<Rep1, Rep2>;
    using CT = measure<Unit, CR, Ratio>;

    return CT(
        CT(lhs).count() * static_cast<CR>(rhs)
    );
}

/**
 * @brief   Multiplication by another `measure`
 */
template <typename Unit, typename Rep1, typename Ratio, typename Rep2>
[[nodiscard]] constexpr
typename std::enable_if<
    std::is_convertible_v<Rep2, typename std::common_type_t<Rep1, Rep2>>,
    measure<Unit, typename std::common_type_t<Rep1, Rep2>, Ratio>
>::type
operator*(const Rep2& lhs, const measure<Unit, Rep1, Ratio>& rhs) noexcept
{
    return rhs * lhs;
}

/**
 * @brief   Multiplication by a different Unit `measure`
 */
template <typename Unit1, typename Rep1, typename Ratio1, typename Unit2, typename Rep2, typename Ratio2>
[[nodiscard]] constexpr
auto
operator*(const measure<Unit1, Rep1, Ratio1>& lhs, const measure<Unit2, Rep2, Ratio2>& rhs) noexcept
{
    using CT =
        typename std::common_type_t<
            measure<Unit1, Rep1, Ratio1>,
            measure<Unit2, Rep2, Ratio2>,
            detail::multiplication_tag
        >;

    return CT(
        CT(lhs).count() * CT(rhs).count()
    );
}

/**
 * @brief   Division by scalar
 */
template <typename Unit, typename Rep1, typename Ratio, typename Rep2>
[[nodiscard]] constexpr
typename std::enable_if<
    !is_measure_v<Rep2> &&
    std::is_convertible_v<Rep2, typename std::common_type_t<Rep1, Rep2>>,
    measure<Unit, typename std::common_type_t<Rep1, Rep2>, Ratio>
>::type
operator/(const measure<Unit, Rep1, Ratio>& lhs, const Rep2& rhs) noexcept
{
    using CR = typename std::common_type_t<Rep1, Rep2>;
    using CT = measure<Unit, CR, Ratio>;

    return CT(
        CT(lhs).count() / static_cast<CR>(rhs)
    );
}

/**
 * @brief   Division by another `measure`
 */
template <typename Unit, typename Rep1, typename Ratio1, typename Rep2, typename Ratio2>
[[nodiscard]] constexpr
auto
operator/(const measure<Unit, Rep1, Ratio1>& lhs, const measure<Unit, Rep2, Ratio2>& rhs) noexcept
{
    using CT =
        typename std::common_type_t<
            measure<Unit, Rep1, Ratio1>,
            measure<Unit, Rep2, Ratio2>
        >;

    return CT(
        CT(lhs).count() / CT(rhs).count()
    );
}

/**
 * @brief   Division by a different Unit `measure`
 */
template <typename Unit1, typename Rep1, typename Ratio1, typename Unit2, typename Rep2, typename Ratio2>
[[nodiscard]] constexpr
auto
operator/(const measure<Unit1, Rep1, Ratio1>& lhs, const measure<Unit2, Rep2, Ratio2>& rhs) noexcept
{
    using CT =
        typename std::common_type_t<
            measure<Unit1, Rep1, Ratio1>,
            measure<Unit2, Rep2, Ratio2>,
            detail::division_tag
        >;

    return CT(
        CT(lhs).count() / CT(rhs).count()
    );
}

template <typename Unit, typename Rep1, typename Ratio, typename Rep2>
[[nodiscard]] constexpr
typename std::enable_if<
    !is_measure_v<Rep2> &&
    std::is_convertible_v<Rep2, typename std::common_type_t<Rep1, Rep2>>,
    measure<Unit, typename std::common_type_t<Rep1, Rep2>, Ratio>
>::type
operator%(const measure<Unit, Rep1, Ratio>& lhs, const Rep2& rhs) noexcept
{
    using CR = typename std::common_type_t<Rep1, Rep2>;
    using CT = measure<Unit, CR, Ratio>;

    return CT(
        CT(lhs).count() % static_cast<CR>(rhs)
    );
}

template <typename Unit1, typename Rep1, typename Ratio1, typename Unit2, typename Rep2, typename Ratio2>
[[nodiscard]] constexpr
typename std::common_type_t<measure<Unit1, Rep1, Ratio1>, measure<Unit2, Rep2, Ratio2>>
operator%(const measure<Unit1, Rep1, Ratio1>& lhs, const measure<Unit2, Rep2, Ratio2>& rhs) noexcept
{
    using CR = typename std::common_type_t<Rep1, Rep2>;
    using CT =
        typename std::common_type_t<
            measure<Unit1, Rep1, Ratio1>,
            measure<Unit2, Rep2, Ratio2>
        >;

    return CT(
        static_cast<CR>(CT(lhs).count())
        % static_cast<CR>(CT(rhs).count())
    );
}

// ------------------------------------==[ Helper Types ]==-----------------------------------------

namespace constants {
    constexpr auto bits_per_byte = 8;
    constexpr auto kByte = 1024;
    constexpr auto MByte = kByte * 1024;
    constexpr auto GByte = MByte * 1024;
    constexpr auto TByte = GByte * 1024LL;

    constexpr auto kilo = 1000;

    constexpr auto mile_to_mm = 1609344;
} // namespace constants

/**
 * @brief   Making sure implicit conversion for the `chrono` adapter
 */
using DefaultRepT = std::chrono::seconds::rep;

using bits   = measure<data_volume, long long, std::ratio<1, constants::bits_per_byte>>;
using bytes  = measure<data_volume, long long>;
using kBytes = measure<data_volume, long long, std::ratio<constants::kByte>>;
using MBytes = measure<data_volume, long long, std::ratio<constants::MByte>>;
using GBytes = measure<data_volume, long long, std::ratio<constants::GByte>>;
using TBytes = measure<data_volume, long long, std::ratio<constants::TByte>>;

using millimeters = measure<length, long long, std::ratio<1, constants::kilo>>;
using meters      = measure<length, long long>;
using kilometers  = measure<length, long long, std::ratio<constants::kilo>>;
using miles       = measure<length, long long, std::ratio<constants::mile_to_mm, constants::kilo>>;

// --------------------------------------==[ Literals ]==-------------------------------------------

namespace literals {

    // Data volume literals

    constexpr auto operator""_bit(unsigned long long x) noexcept {
        return bits(static_cast<bits::Rep>(x));
    }

    constexpr auto operator""_byte(unsigned long long x) noexcept {
        return bytes(static_cast<bytes::Rep>(x));
    }

    constexpr auto operator""_kB(unsigned long long x) noexcept {
        return kBytes(static_cast<kBytes::Rep>(x));
    }

    constexpr auto operator""_MB(unsigned long long x) noexcept {
        return MBytes(static_cast<MBytes::Rep>(x));
    }

    constexpr auto operator""_GB(unsigned long long x) noexcept {
        return GBytes(static_cast<GBytes::Rep>(x));
    }

    constexpr auto operator""_TB(unsigned long long x) noexcept {
        return TBytes(static_cast<TBytes::Rep>(x));
    }

    // Length literals

    constexpr auto operator""_mm(unsigned long long x) noexcept {
        return millimeters(static_cast<millimeters::Rep>(x));
    }

    constexpr auto operator""_m(unsigned long long x) noexcept {
        return meters(static_cast<meters::Rep>(x));
    }

    constexpr auto operator""_km(unsigned long long x) noexcept {
        return kilometers(static_cast<kilometers::Rep>(x));
    }

    constexpr auto operator""_mi(unsigned long long x) noexcept {
        return miles(static_cast<miles::Rep>(x));
    }

} // namespace literals

} // namespace nova::units
