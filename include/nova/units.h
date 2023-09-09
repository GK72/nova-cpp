#pragma once

#include <numeric>
#include <ratio>

namespace nova::units {

template <typename Rep, typename Ratio = std::ratio<1>> class measurement;

namespace detail {
    template <typename R1, typename R2>
    struct ratioGCD
    {
        using type =
            std::ratio<std::gcd(R1::num, R2::num),
                       std::lcm(R1::den, R2::den)>;
    };
} // namespace detail
} // namespace nova::units

// -----------------------------==[ Common_Type Specialization ]==----------------------------------

namespace std {
    template <typename Rep1, typename Ratio1, typename Rep2, typename Ratio2>
    struct common_type<
        nova::units::measurement<Rep1, Ratio1>,
        nova::units::measurement<Rep2, Ratio2>>
    {
        using type = nova::units::measurement<
            typename common_type<Rep1, Rep2>::type,
            typename nova::units::detail::ratioGCD<Ratio1, Ratio2>::type>;
    };
} // namespace std

namespace nova::units {

// -------------------------------------==[ Type Trait ]==------------------------------------------

template <typename T>
struct is_measurement : std::false_type {};

template <typename Rep, typename Ratio>
struct is_measurement<measurement<Rep, Ratio>> : std::true_type {};

template <typename Rep, typename Ratio>
struct is_measurement<const measurement<Rep, Ratio>> : std::true_type {};

template <typename Rep, typename Ratio>
struct is_measurement<volatile measurement<Rep, Ratio>> : std::true_type {};

template <typename Rep, typename Ratio>
struct is_measurement<const volatile measurement<Rep, Ratio>> : std::true_type {};

template <typename T>
constexpr bool is_measurement_v = is_measurement<T>::value;

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
    template <typename FromMeasurement, typename ToMeasurement,
              typename Ratio = typename std::ratio_divide<
                    typename FromMeasurement::Ratio,
                    typename ToMeasurement::Ratio>::type,
              bool = Ratio::num == 1,
              bool = Ratio::den == 1>
    struct measurement_cast;

    /**
     * @brief Representation casting
     */
    template <typename FromMeasurement, typename ToMeasurement, typename Ratio>
    struct measurement_cast<FromMeasurement, ToMeasurement, Ratio, /* num = 1 */ true, /* den = 1 */ true>
    {
        constexpr ToMeasurement operator()(const FromMeasurement& x) const noexcept {
            return ToMeasurement(static_cast<typename ToMeasurement::Rep>(x.count()));
        }
    };

    /**
     * @brief Representation casting and Ratio down conversion
     */
    template <typename FromMeasurement, typename ToMeasurement, typename Ratio>
    struct measurement_cast<FromMeasurement, ToMeasurement, Ratio, /* num = 1 */ true, /* den = 1 */ false>
    {
        constexpr ToMeasurement operator()(const FromMeasurement& x) const noexcept {
            using CT =
                typename std::common_type_t<
                    typename ToMeasurement::Rep,
                    typename FromMeasurement::Rep,
                    intmax_t>;
            return ToMeasurement(static_cast<typename ToMeasurement::Rep>(
                static_cast<CT>(x.count()) / static_cast<CT>(Ratio::den))
            );
        }
    };

    /**
     * @brief Representation casting and Ratio up conversion
     */
    template <typename FromMeasurement, typename ToMeasurement, typename Ratio>
    struct measurement_cast<FromMeasurement, ToMeasurement, Ratio, /* num = 1 */ false, /* den = 1 */ true>
    {
        constexpr ToMeasurement operator()(const FromMeasurement& x) const noexcept {
            using CT =
                typename std::common_type_t<
                    typename ToMeasurement::Rep,
                    typename FromMeasurement::Rep,
                    intmax_t>;
            return ToMeasurement(static_cast<typename ToMeasurement::Rep>(
                static_cast<CT>(x.count()) * static_cast<CT>(Ratio::num))
            );
        }
    };

    /**
     * @brief Representation casting and Ratio conversion
     */
    template <typename FromMeasurement, typename ToMeasurement, typename Ratio>
    struct measurement_cast<FromMeasurement, ToMeasurement, Ratio, /* num = 1 */ false, /* den = 1 */ false>
    {
        constexpr ToMeasurement operator()(const FromMeasurement& x) const noexcept {
            using CT =
                typename std::common_type_t<
                    typename ToMeasurement::Rep,
                    typename FromMeasurement::Rep,
                    intmax_t>;;
            return ToMeasurement(static_cast<typename ToMeasurement::Rep>(
                static_cast<CT>(x.count())
                    * static_cast<CT>(Ratio::num)
                    / static_cast<CT>(Ratio::den))
            );
        }
    };

    // ----------------------==[ Relational operator implementations ]==----------------------------

    template <typename Lhs, typename Rhs>             // ----==[ Different types ]==----
    struct measurement_eq {
        [[nodiscard]] constexpr
        bool operator()(const Lhs& lhs, const Rhs& rhs) const noexcept {
            using CT = typename std::common_type_t<Lhs, Rhs>;
            return CT(lhs).count() == CT(rhs).count();
        }
    };

    template <typename Lhs, typename Rhs>
    struct measurement_lt {
        [[nodiscard]] constexpr
        bool operator()(const Lhs& lhs, const Rhs& rhs) const noexcept {
            using CT = typename std::common_type_t<Lhs, Rhs>;
            return CT(lhs).count() < CT(rhs).count();
        }
    };

    template <typename Lhs>                        // ----==[ Common type ]==----
    struct measurement_eq<Lhs, Lhs> {
        [[nodiscard]] constexpr
        bool operator()(const Lhs& lhs, const Lhs& rhs) const noexcept {
            return lhs.count() == rhs.count();
        }
    };

    template <typename Lhs>
    struct measurement_lt<Lhs, Lhs> {
        [[nodiscard]] constexpr
        bool operator()(const Lhs& lhs, const Lhs& rhs) const noexcept {
            return lhs.count() < rhs.count();
        }
    };

    template <typename Rep>
    struct measurement_values {
        static constexpr Rep zero() noexcept { return Rep(0); }
        static constexpr Rep min()  noexcept { return std::numeric_limits<Rep>::lowest(); }
        static constexpr Rep max()  noexcept { return std::numeric_limits<Rep>::max(); }
    };

} // namespace detail

/**
 * @brief Wrapper for casting
 */
template <typename ToMeasurement, typename Rep, typename Ratio>
[[nodiscard]] constexpr
typename std::enable_if_t<is_measurement_v<ToMeasurement>, ToMeasurement>
measurement_cast(const measurement<Rep, Ratio>& x) noexcept
{
    return detail::measurement_cast<measurement<Rep, Ratio>, ToMeasurement>()(x);
}

// -------------------------------------==[ UNIT CLASS ]==------------------------------------------

template <typename RepT, typename RatioT>
class measurement {
    static_assert(!is_measurement_v<RepT>, "A measurement representation can not be a measurement");
    static_assert(is_ratio_v<RatioT>, "Second template parameter must be a std::ratio");
    static_assert(RatioT::num > 0, "Ratio must be positive");

public:
    using Ratio = typename RatioT::type;
    using Rep     = RepT;

    constexpr measurement() = default;

    template <typename Rep2>
    constexpr explicit measurement(const Rep2& value,
        typename std::enable_if_t<
            std::is_convertible_v<Rep2, Rep>
            && (std::is_floating_point_v<Rep>
            || !std::is_floating_point_v<Rep2>)
        >* = 0      // NOLINT(readability-named-parameter)
    )
        : m_value(value)
    {}

    template <typename Rep2, typename Ratio2>
    constexpr measurement(const measurement<Rep2, Ratio2>& value,
        typename std::enable_if_t<
            std::is_floating_point_v<Rep>
            || (std::ratio_divide<Ratio2, Ratio>::den == 1
            && !std::is_floating_point_v<Rep2>)
        >* = 0      // NOLINT(readability-named-parameter)
    )
        : m_value(measurement_cast<measurement>(value).count())
    {}

    constexpr Rep count() const { return m_value; }

    typename std::common_type_t<measurement> operator+() const {
        return typename std::common_type_t<measurement>(*this);
    }

    typename std::common_type_t<measurement> operator-() const {
        return typename std::common_type_t<measurement>(-m_value);
    }

    constexpr measurement  operator++(int) { return measurement(m_value++);  }
    constexpr measurement  operator--(int) { return measurement(m_value--);  }
    constexpr measurement& operator++()    { ++m_value; return *this; }
    constexpr measurement& operator--()    { --m_value; return *this; }

    constexpr measurement& operator+=(const measurement& rhs) { m_value += rhs.count(); return *this; }
    constexpr measurement& operator-=(const measurement& rhs) { m_value -= rhs.count(); return *this; }
    constexpr measurement& operator*=(const Rep& rhs)  { m_value *= rhs;         return *this; }
    constexpr measurement& operator/=(const Rep& rhs)  { m_value /= rhs;         return *this; }
    constexpr measurement& operator%=(const Rep& rhs)  { m_value %= rhs;         return *this; }
    constexpr measurement& operator%=(const measurement& rhs) { m_value += rhs.count(); return *this; }

    static constexpr measurement zero() noexcept { return measurement(detail::measurement_values<Rep>::zero()); }
    static constexpr measurement min()  noexcept { return measurement(detail::measurement_values<Rep>::min()); }
    static constexpr measurement max()  noexcept { return measurement(detail::measurement_values<Rep>::max()); }

private:
    Rep m_value;

};

// --------------------------------==[ Relational operators ]==-------------------------------------

template <typename Rep1, typename Ratio1, typename Rep2, typename Ratio2>
[[nodiscard]] constexpr
bool operator==(const measurement<Rep1, Ratio1>& lhs, const measurement<Rep2, Ratio2>& rhs) noexcept {
    return detail::measurement_eq<measurement<Rep1, Ratio1>, measurement<Rep2, Ratio2>>()(lhs, rhs);
};

template <typename Rep1, typename Ratio1, typename Rep2, typename Ratio2>
[[nodiscard]] constexpr
bool operator!=(const measurement<Rep1, Ratio1>& lhs, const measurement<Rep2, Ratio2>& rhs) noexcept {
    return !(lhs == rhs);
};

template <typename Rep1, typename Ratio1, typename Rep2, typename Ratio2>
[[nodiscard]] constexpr
bool operator<(const measurement<Rep1, Ratio1>& lhs, const measurement<Rep2, Ratio2>& rhs) noexcept {
    return detail::measurement_lt<measurement<Rep1, Ratio1>, measurement<Rep2, Ratio2>>()(lhs, rhs);
};

template <typename Rep1, typename Ratio1, typename Rep2, typename Ratio2>
[[nodiscard]] constexpr
bool operator>(const measurement<Rep1, Ratio1>& lhs, const measurement<Rep2, Ratio2>& rhs) noexcept {
    return rhs < lhs;
};

template <typename Rep1, typename Ratio1, typename Rep2, typename Ratio2>
[[nodiscard]] constexpr
bool operator>=(const measurement<Rep1, Ratio1>& lhs, const measurement<Rep2, Ratio2>& rhs) noexcept {
    return !(lhs < rhs);
};

template <typename Rep1, typename Ratio1, typename Rep2, typename Ratio2>
[[nodiscard]] constexpr
bool operator<=(const measurement<Rep1, Ratio1>& lhs, const measurement<Rep2, Ratio2>& rhs) noexcept {
    return !(lhs > rhs);
};

// --------------------------------==[ Arithmetic operators ]==-------------------------------------

template <typename Rep1, typename Ratio1, typename Rep2, typename Ratio2>
[[nodiscard]] constexpr
typename std::common_type_t<measurement<Rep1, Ratio1>, measurement<Rep2, Ratio2>>
operator+(const measurement<Rep1, Ratio1>& lhs, const measurement<Rep2, Ratio2>& rhs) noexcept
{
    using CT =
        typename std::common_type_t<
            measurement<Rep1, Ratio1>,
            measurement<Rep2, Ratio2>
        >;

    return CT(
        CT(lhs).count() + CT(rhs).count()
    );
}

template <typename Rep1, typename Ratio1, typename Rep2, typename Ratio2>
[[nodiscard]] constexpr
typename std::common_type_t<measurement<Rep1, Ratio1>, measurement<Rep2, Ratio2>>
operator-(const measurement<Rep1, Ratio1>& lhs, const measurement<Rep2, Ratio2>& rhs) noexcept
{
    using CT =
        typename std::common_type_t<
            measurement<Rep1, Ratio1>,
            measurement<Rep2, Ratio2>
        >;

    return CT(
        CT(lhs).count() - CT(rhs).count()
    );
}

template <typename Rep1, typename Ratio, typename Rep2>
[[nodiscard]] constexpr
typename std::enable_if<
    std::is_convertible_v<Rep2, typename std::common_type_t<Rep1, Rep2>>,
    measurement<typename std::common_type_t<Rep1, Rep2>, Ratio>
>::type
operator*(const measurement<Rep1, Ratio>& lhs, const Rep2& rhs) noexcept
{
    using CR = typename std::common_type_t<Rep1, Rep2>;
    using CT = measurement<CR, Ratio>;

    return CT(
        CT(lhs).count() * static_cast<CR>(rhs)
    );
}

template <typename Rep1, typename Ratio, typename Rep2>
[[nodiscard]] constexpr
typename std::enable_if<
    std::is_convertible_v<Rep2, typename std::common_type_t<Rep1, Rep2>>,
    measurement<typename std::common_type_t<Rep1, Rep2>, Ratio>
>::type
operator*(const Rep2& lhs, const measurement<Rep1, Ratio>& rhs) noexcept
{
    return rhs * lhs;
}

template <typename Rep1, typename Ratio, typename Rep2>
[[nodiscard]] constexpr
typename std::enable_if<
    !is_measurement_v<Rep2> &&
    std::is_convertible_v<Rep2, typename std::common_type_t<Rep1, Rep2>>,
    measurement<typename std::common_type_t<Rep1, Rep2>, Ratio>
>::type
operator/(const measurement<Rep1, Ratio>& lhs, const Rep2& rhs) noexcept
{
    using CR = typename std::common_type_t<Rep1, Rep2>;
    using CT = measurement<CR, Ratio>;

    return CT(
        CT(lhs).count() / static_cast<CR>(rhs)
    );
}

template <typename Rep1, typename Ratio1, typename Rep2, typename Ratio2>
[[nodiscard]] constexpr
typename std::common_type_t<Rep1, Rep2>
operator/(const measurement<Rep1, Ratio1>& lhs, const measurement<Rep1, Ratio1>& rhs) noexcept
{
    using CT =
        typename std::common_type_t<
            measurement<Rep1, Ratio1>,
            measurement<Rep2, Ratio2>
        >;

    return CT(
        CT(lhs).count() / CT(rhs).count()
    );
}

template <typename Rep1, typename Ratio, typename Rep2>
[[nodiscard]] constexpr
typename std::enable_if<
    !is_measurement_v<Rep2> &&
    std::is_convertible_v<Rep2, typename std::common_type_t<Rep1, Rep2>>,
    measurement<typename std::common_type_t<Rep1, Rep2>, Ratio>
>::type
operator%(const measurement<Rep1, Ratio>& lhs, const Rep2& rhs) noexcept
{
    using CR = typename std::common_type_t<Rep1, Rep2>;
    using CT = measurement<CR, Ratio>;

    return CT(
        CT(lhs).count() % static_cast<CR>(rhs)
    );
}

template <typename Rep1, typename Ratio1, typename Rep2, typename Ratio2>
[[nodiscard]] constexpr
typename std::common_type_t<measurement<Rep1, Ratio1>, measurement<Rep2, Ratio2>>
operator%(const measurement<Rep1, Ratio1>& lhs, const measurement<Rep2, Ratio2>& rhs) noexcept
{
    using CR = typename std::common_type_t<Rep1, Rep2>;
    using CT =
        typename std::common_type_t<
            measurement<Rep1, Ratio1>,
            measurement<Rep2, Ratio2>
        >;

    return CT(
        static_cast<CR>(CT(lhs).count())
        % static_cast<CR>(CT(rhs).count())
    );
}

// ------------------------------------==[ Helper Types ]==-----------------------------------------

namespace constants {
    constexpr auto bit = 8;
    constexpr auto kByte = 1024;
    constexpr auto MByte = kByte * 1024;
    constexpr auto GByte = MByte * 1024;
    constexpr auto TByte = GByte * 1024LL;
} // namespace constants

using bit   = measurement<long long, std::ratio<1, constants::bit>>;
using byte  = measurement<long long>;
using kByte = measurement<long long, std::ratio<constants::kByte>>;
using MByte = measurement<long long, std::ratio<constants::MByte>>;
using GByte = measurement<long long, std::ratio<constants::GByte>>;
using TByte = measurement<long long, std::ratio<constants::TByte>>;

// --------------------------------------==[ Literals ]==-------------------------------------------

namespace literals {
    constexpr auto operator""_bit(unsigned long long x) noexcept {
        return bit(static_cast<bit::Rep>(x));
    }

    constexpr auto operator""_byte(unsigned long long x) noexcept {
        return byte(static_cast<byte::Rep>(x));
    }

    constexpr auto operator""_kB(unsigned long long x) noexcept {
        return kByte(static_cast<kByte::Rep>(x));
    }

    constexpr auto operator""_MB(unsigned long long x) noexcept {
        return MByte(static_cast<MByte::Rep>(x));
    }

    constexpr auto operator""_GB(unsigned long long x) noexcept {
        return GByte(static_cast<GByte::Rep>(x));
    }

    constexpr auto operator""_TB(unsigned long long x) noexcept {
        return TByte(static_cast<TByte::Rep>(x));
    }
} // namespace literals

} // namespace nova::units
