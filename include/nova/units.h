#pragma once

#include <numeric>
#include <ratio>

namespace nova::units {

template <typename Rep, typename Measure = std::ratio<1>> class unit;

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
    template <typename Rep1, typename Measure1, typename Rep2, typename Measure2>
    struct common_type<
        nova::units::unit<Rep1, Measure1>,
        nova::units::unit<Rep2, Measure2>>
    {
        using type = nova::units::unit<
            typename common_type<Rep1, Rep2>::type,
            typename nova::units::detail::ratioGCD<Measure1, Measure2>::type>;
    };
} // namespace std

namespace nova::units {

// -------------------------------------==[ Type Trait ]==------------------------------------------

template <typename T>
struct is_unit : std::false_type {};

template <typename Rep, typename Measure>
struct is_unit<unit<Rep, Measure>> : std::true_type {};

template <typename Rep, typename Measure>
struct is_unit<const unit<Rep, Measure>> : std::true_type {};

template <typename Rep, typename Measure>
struct is_unit<volatile unit<Rep, Measure>> : std::true_type {};

template <typename Rep, typename Measure>
struct is_unit<const volatile unit<Rep, Measure>> : std::true_type {};

template <typename T>
constexpr bool is_unit_v = is_unit<T>::value;

template <typename T>
struct is_ratio : std::false_type {};

template <intmax_t N, intmax_t D>
struct is_ratio<std::ratio<N, D>> : std::true_type {};

// --------------------------------------==[ Casting ]==--------------------------------------------

namespace detail {

    /**
     * @brief No conversion
     */
    template <typename FromUnit, typename ToUnit,
              typename Measure = typename std::ratio_divide<
                    typename FromUnit::Measure,
                    typename ToUnit::Measure>::type,
              bool = Measure::num == 1,
              bool = Measure::den == 1>
    struct unit_cast;

    /**
     * @brief Representation casting
     */
    template <typename FromUnit, typename ToUnit, typename Measure>
    struct unit_cast<FromUnit, ToUnit, Measure, /* num = 1 */ true, /* den = 1 */ true>
    {
        constexpr ToUnit operator()(const FromUnit& x) const noexcept {
            return ToUnit(static_cast<typename ToUnit::Rep>(x.count()));
        }
    };

    /**
     * @brief Representation casting and Measure down conversion
     */
    template <typename FromUnit, typename ToUnit, typename Measure>
    struct unit_cast<FromUnit, ToUnit, Measure, /* num = 1 */ true, /* den = 1 */ false>
    {
        constexpr ToUnit operator()(const FromUnit& x) const noexcept {
            using CT =
                typename std::common_type_t<
                    typename ToUnit::Rep,
                    typename FromUnit::Rep,
                    intmax_t>;
            return ToUnit(static_cast<typename ToUnit::Rep>(
                static_cast<CT>(x.count()) / static_cast<CT>(Measure::den))
            );
        }
    };

    /**
     * @brief Representation casting and Measure up conversion
     */
    template <typename FromUnit, typename ToUnit, typename Measure>
    struct unit_cast<FromUnit, ToUnit, Measure, /* num = 1 */ false, /* den = 1 */ true>
    {
        constexpr ToUnit operator()(const FromUnit& x) const noexcept {
            using CT =
                typename std::common_type_t<
                    typename ToUnit::Rep,
                    typename FromUnit::Rep,
                    intmax_t>;
            return ToUnit(static_cast<typename ToUnit::Rep>(
                static_cast<CT>(x.count()) * static_cast<CT>(Measure::num))
            );
        }
    };

    /**
     * @brief Representation casting and Measure conversion
     */
    template <typename FromUnit, typename ToUnit, typename Measure>
    struct unit_cast<FromUnit, ToUnit, Measure, /* num = 1 */ false, /* den = 1 */ false>
    {
        constexpr ToUnit operator()(const FromUnit& x) const noexcept {
            using CT =
                typename std::common_type_t<
                    typename ToUnit::Rep,
                    typename FromUnit::Rep,
                    intmax_t>;;
            return ToUnit(static_cast<typename ToUnit::Rep>(
                static_cast<CT>(x.count())
                    * static_cast<CT>(Measure::num)
                    / static_cast<CT>(Measure::den))
            );
        }
    };

    // ----------------------==[ Relational operator implementations ]==----------------------------

    template <typename Lhs, typename Rhs>             // ----==[ Different types ]==----
    struct unit_eq {
        [[nodiscard]] constexpr
        bool operator()(const Lhs& lhs, const Rhs& rhs) const noexcept {
            using CT = typename std::common_type_t<Lhs, Rhs>;
            return CT(lhs).count() == CT(rhs).count();
        }
    };

    template <typename Lhs, typename Rhs>
    struct unit_lt {
        [[nodiscard]] constexpr
        bool operator()(const Lhs& lhs, const Rhs& rhs) const noexcept {
            using CT = typename std::common_type_t<Lhs, Rhs>;
            return CT(lhs).count() < CT(rhs).count();
        }
    };

    template <typename Lhs>                        // ----==[ Common type ]==----
    struct unit_eq<Lhs, Lhs> {
        [[nodiscard]] constexpr
        bool operator()(const Lhs& lhs, const Lhs& rhs) const noexcept {
            return lhs.count() == rhs.count();
        }
    };

    template <typename Lhs>
    struct unit_lt<Lhs, Lhs> {
        [[nodiscard]] constexpr
        bool operator()(const Lhs& lhs, const Lhs& rhs) const noexcept {
            return lhs.count() < rhs.count();
        }
    };

    template <typename Rep>
    struct unit_values {
        static constexpr Rep zero() noexcept { return Rep(0); }
        static constexpr Rep min()  noexcept { return std::numeric_limits<Rep>::lowest(); }
        static constexpr Rep max()  noexcept { return std::numeric_limits<Rep>::max(); }
    };

} // namespace detail

/**
 * @brief Wrapper for casting
 */
template <typename ToUnit, typename Rep, typename Measure>
[[nodiscard]] constexpr
typename std::enable_if_t<is_unit_v<ToUnit>, ToUnit>
unit_cast(const unit<Rep, Measure>& x) noexcept
{
    return detail::unit_cast<unit<Rep, Measure>, ToUnit>()(x);
}

// -------------------------------------==[ UNIT CLASS ]==------------------------------------------

template <typename RepT, typename MeasureT>
class unit {
    static_assert(!is_unit<RepT>::value, "A unit representation can not be a unit");
    static_assert(is_ratio<MeasureT>::value, "Second template parameter must be a std::ratio");
    static_assert(MeasureT::num > 0, "Measure must be positive");

public:
    using Measure = typename MeasureT::type;
    using Rep     = RepT;

    constexpr unit() = default;

    template <typename Rep2>
    constexpr explicit unit(const Rep2& value,
        typename std::enable_if_t<
            std::is_convertible_v<Rep2, Rep>
            && (std::is_floating_point_v<Rep>
            || !std::is_floating_point_v<Rep2>)
        >* = 0      // NOLINT(readability-named-parameter)
    )
        : m_value(value)
    {}

    template <typename Rep2, typename Measure2>
    constexpr unit(const unit<Rep2, Measure2>& value,
        typename std::enable_if_t<
            std::is_floating_point_v<Rep>
            || (std::ratio_divide<Measure2, Measure>::den == 1
            && !std::is_floating_point_v<Rep2>)
        >* = 0      // NOLINT(readability-named-parameter)
    )
        : m_value(unit_cast<unit>(value).count())
    {}

    constexpr Rep count() const { return m_value; }

    typename std::common_type_t<unit> operator+() const {
        return typename std::common_type_t<unit>(*this);
    }

    typename std::common_type_t<unit> operator-() const {
        return typename std::common_type_t<unit>(-m_value);
    }

    constexpr unit  operator++(int) { return unit(m_value++);  }
    constexpr unit  operator--(int) { return unit(m_value--);  }
    constexpr unit& operator++()    { ++m_value; return *this; }
    constexpr unit& operator--()    { --m_value; return *this; }

    constexpr unit& operator+=(const unit& rhs) { m_value += rhs.count(); return *this; }
    constexpr unit& operator-=(const unit& rhs) { m_value -= rhs.count(); return *this; }
    constexpr unit& operator*=(const Rep& rhs)  { m_value *= rhs;         return *this; }
    constexpr unit& operator/=(const Rep& rhs)  { m_value /= rhs;         return *this; }
    constexpr unit& operator%=(const Rep& rhs)  { m_value %= rhs;         return *this; }
    constexpr unit& operator%=(const unit& rhs) { m_value += rhs.count(); return *this; }

    static constexpr unit zero() noexcept { return unit(detail::unit_values<Rep>::zero()); }
    static constexpr unit min()  noexcept { return unit(detail::unit_values<Rep>::min()); }
    static constexpr unit max()  noexcept { return unit(detail::unit_values<Rep>::max()); }

private:
    Rep m_value;

};

// --------------------------------==[ Relational operators ]==-------------------------------------

template <typename Rep1, typename Measure1, typename Rep2, typename Measure2>
[[nodiscard]] constexpr
bool operator==(const unit<Rep1, Measure1>& lhs, const unit<Rep2, Measure2>& rhs) noexcept {
    return detail::unit_eq<unit<Rep1, Measure1>, unit<Rep2, Measure2>>()(lhs, rhs);
};

template <typename Rep1, typename Measure1, typename Rep2, typename Measure2>
[[nodiscard]] constexpr
bool operator!=(const unit<Rep1, Measure1>& lhs, const unit<Rep2, Measure2>& rhs) noexcept {
    return !(lhs == rhs);
};

template <typename Rep1, typename Measure1, typename Rep2, typename Measure2>
[[nodiscard]] constexpr
bool operator<(const unit<Rep1, Measure1>& lhs, const unit<Rep2, Measure2>& rhs) noexcept {
    return detail::unit_lt<unit<Rep1, Measure1>, unit<Rep2, Measure2>>()(lhs, rhs);
};

template <typename Rep1, typename Measure1, typename Rep2, typename Measure2>
[[nodiscard]] constexpr
bool operator>(const unit<Rep1, Measure1>& lhs, const unit<Rep2, Measure2>& rhs) noexcept {
    return rhs < lhs;
};

template <typename Rep1, typename Measure1, typename Rep2, typename Measure2>
[[nodiscard]] constexpr
bool operator>=(const unit<Rep1, Measure1>& lhs, const unit<Rep2, Measure2>& rhs) noexcept {
    return !(lhs < rhs);
};

template <typename Rep1, typename Measure1, typename Rep2, typename Measure2>
[[nodiscard]] constexpr
bool operator<=(const unit<Rep1, Measure1>& lhs, const unit<Rep2, Measure2>& rhs) noexcept {
    return !(lhs > rhs);
};

// --------------------------------==[ Arithmetic operators ]==-------------------------------------

template <typename Rep1, typename Measure1, typename Rep2, typename Measure2>
[[nodiscard]] constexpr
typename std::common_type_t<unit<Rep1, Measure1>, unit<Rep2, Measure2>>
operator+(const unit<Rep1, Measure1>& lhs, const unit<Rep2, Measure2>& rhs) noexcept
{
    using CT =
        typename std::common_type_t<
            unit<Rep1, Measure1>,
            unit<Rep2, Measure2>
        >;

    return CT(
        CT(lhs).count() + CT(rhs).count()
    );
}

template <typename Rep1, typename Measure1, typename Rep2, typename Measure2>
[[nodiscard]] constexpr
typename std::common_type_t<unit<Rep1, Measure1>, unit<Rep2, Measure2>>
operator-(const unit<Rep1, Measure1>& lhs, const unit<Rep2, Measure2>& rhs) noexcept
{
    using CT =
        typename std::common_type_t<
            unit<Rep1, Measure1>,
            unit<Rep2, Measure2>
        >;

    return CT(
        CT(lhs).count() - CT(rhs).count()
    );
}

template <typename Rep1, typename Measure, typename Rep2>
[[nodiscard]] constexpr
typename std::enable_if<
    std::is_convertible_v<Rep2, typename std::common_type_t<Rep1, Rep2>>,
    unit<typename std::common_type_t<Rep1, Rep2>, Measure>
>::type
operator*(const unit<Rep1, Measure>& lhs, const Rep2& rhs) noexcept
{
    using CR = typename std::common_type_t<Rep1, Rep2>;
    using CT = unit<CR, Measure>;

    return CT(
        CT(lhs).count() * static_cast<CR>(rhs)
    );
}

template <typename Rep1, typename Measure, typename Rep2>
[[nodiscard]] constexpr
typename std::enable_if<
    std::is_convertible_v<Rep2, typename std::common_type_t<Rep1, Rep2>>,
    unit<typename std::common_type_t<Rep1, Rep2>, Measure>
>::type
operator*(const Rep2& lhs, const unit<Rep1, Measure>& rhs) noexcept
{
    return rhs * lhs;
}

template <typename Rep1, typename Measure, typename Rep2>
[[nodiscard]] constexpr
typename std::enable_if<
    !is_unit_v<Rep2> &&
    std::is_convertible_v<Rep2, typename std::common_type_t<Rep1, Rep2>>,
    unit<typename std::common_type_t<Rep1, Rep2>, Measure>
>::type
operator/(const unit<Rep1, Measure>& lhs, const Rep2& rhs) noexcept
{
    using CR = typename std::common_type_t<Rep1, Rep2>;
    using CT = unit<CR, Measure>;

    return CT(
        CT(lhs).count() / static_cast<CR>(rhs)
    );
}

template <typename Rep1, typename Measure1, typename Rep2, typename Measure2>
[[nodiscard]] constexpr
typename std::common_type_t<Rep1, Rep2>
operator/(const unit<Rep1, Measure1>& lhs, const unit<Rep1, Measure1>& rhs) noexcept
{
    using CT =
        typename std::common_type_t<
            unit<Rep1, Measure1>,
            unit<Rep2, Measure2>
        >;

    return CT(
        CT(lhs).count() / CT(rhs).count()
    );
}

template <typename Rep1, typename Measure, typename Rep2>
[[nodiscard]] constexpr
typename std::enable_if<
    !is_unit_v<Rep2> &&
    std::is_convertible_v<Rep2, typename std::common_type_t<Rep1, Rep2>>,
    unit<typename std::common_type_t<Rep1, Rep2>, Measure>
>::type
operator%(const unit<Rep1, Measure>& lhs, const Rep2& rhs) noexcept
{
    using CR = typename std::common_type_t<Rep1, Rep2>;
    using CT = unit<CR, Measure>;

    return CT(
        CT(lhs).count() % static_cast<CR>(rhs)
    );
}

template <typename Rep1, typename Measure1, typename Rep2, typename Measure2>
[[nodiscard]] constexpr
typename std::common_type_t<unit<Rep1, Measure1>, unit<Rep2, Measure2>>
operator%(const unit<Rep1, Measure1>& lhs, const unit<Rep2, Measure2>& rhs) noexcept
{
    using CR = typename std::common_type_t<Rep1, Rep2>;
    using CT =
        typename std::common_type_t<
            unit<Rep1, Measure1>,
            unit<Rep2, Measure2>
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

using bit   = unit<long long, std::ratio<1, constants::bit>>;
using byte  = unit<long long>;
using kByte = unit<long long, std::ratio<constants::kByte>>;
using MByte = unit<long long, std::ratio<constants::MByte>>;
using GByte = unit<long long, std::ratio<constants::GByte>>;
using TByte = unit<long long, std::ratio<constants::TByte>>;

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
