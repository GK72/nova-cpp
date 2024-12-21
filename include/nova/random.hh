#pragma once

#include "nova/error.hh"
#include "nova/types.hh"
#include "nova/utils.hh"

#include <concepts>
#include <numeric>
#include <random>
#include <ranges>
#include <type_traits>

namespace nova {

namespace detail {

    /**
     * @brief   Generate a number in the given `range`
     *
     * @precondition    `range`'s `low` value cannot be greater than `high`
     */
    template <typename T>
        requires std::conjunction_v<
            std::is_arithmetic<T>,
            std::negation<std::is_same<T, bool>>
        >
    auto generate_number(range<T> r, std::uniform_random_bit_generator auto& gen) -> T {
        nova_assert(r.low <= r.high);                                                                   // NOLINT(cppcoreguidelines-pro-bounds-array-to-pointer-decay) | False positive

        if constexpr (std::is_integral_v<T>) {
            return std::uniform_int_distribution<T>(r.low, r.high)(gen);
        }
        else if constexpr (std::is_floating_point_v<T>) {
            return std::uniform_real_distribution<T>(r.low, r.high)(gen);
        }
    }

    /**
     * @brief   Pick a random element from a `range`
     */
    template <std::ranges::range Range>
    auto pick_from(const Range& elements, std::uniform_random_bit_generator auto& gen)
            -> typename Range::value_type
    {
        using difference_type = typename Range::difference_type;

        const auto idx = generate_number(range<std::size_t>{ 0, std::size(elements) - 1 }, gen);
        return *std::next(std::begin(elements), static_cast<difference_type>(idx));
    }

} // namespace detail

struct ascii_distribution {
    [[nodiscard]] char operator()(std::uniform_random_bit_generator auto& gen) {
        return static_cast<char>(
            std::uniform_int_distribution(
                static_cast<int>(ascii::PrintableRange.low),
                static_cast<int>(ascii::PrintableRange.high)
            )(gen)
        );
    }
};

struct alphanumeric_distribution {
    [[nodiscard]] char operator()(std::uniform_random_bit_generator auto& gen) {
        return detail::pick_from(
            concat(
                ascii::lowercase_letters(),
                ascii::uppercase_letters(),
                ascii::numbers()
            ),
            gen
        );
    }
};

struct alphabetic_distribution {
    [[nodiscard]] char operator()(std::uniform_random_bit_generator auto& gen) {
        return detail::pick_from(
            concat(
                ascii::lowercase_letters(),
                ascii::uppercase_letters()
            ),
            gen
        );
    }
};

/**
 * @brief   A convenience class to generate random things
 *
 * Contains the engine and seed with some wrapper member functions.
 *
 * CAUTION: Do NOT use for crpytographic applications!
 */
class rng {
public:
    /**
     * @brief   Initialize engine with a random seed
     */
    rng()                                                                                           // NOLINT(cert-msc32-c,cert-msc51-cpp) | Uses `std::random_device` for seeding the engine
        : m_seed(m_rd())
    {
        m_mt.seed(m_seed);
    }

    /**
     * @brief   Initialize engine with a given seed
     *
     * CAUTION: Use only when reproducible results are needed, e.g. testing / simulation
     */
    rng(std::random_device::result_type seed)                                                       // NOLINT(cert-msc32-c,cert-msc51-cpp) | Documented
        : m_seed(seed)
    {
        m_mt.seed(m_seed);
    }

    /**
     * @brief   For saving the seed to be able to reproduce the results
     */
    [[nodiscard]] auto seed() const noexcept { return m_seed; }

    /**
     * @brief   Pick a random element from a `range`
     */
    template <std::ranges::range Range>
    auto choice(const Range& r) -> typename Range::value_type {
        return detail::pick_from(r, m_mt);
    }

    /**
     * @brief   Generate a number in the given `range`
     */
    template <typename T>
        requires std::conjunction_v<
            std::is_arithmetic<T>,
            std::negation<std::is_same<T, bool>>
        >
    auto number(range<T> r) -> T {
        return detail::generate_number(r, m_mt);
    }

    /**
     * @brief   Generate a number in the range [0, 1)
     */
    auto number() -> double {
        return detail::generate_number(range<double>{ 0.0, 1.0 }, m_mt);
    }

    /**
     * @brief   Generate a given `length` string according to `Distribution`
     *
     * `Distribution` must be a callable with a type that is valid for
     * `std::uniform_random_bit_generator` concept
     */
    template <typename Distribution>
    auto string(std::size_t length) -> std::string {
        std::string result(length, '\0');

        for (auto& ch : result) {
            ch = Distribution()(m_mt);
        }

        return result;
    }

private:
    std::random_device m_rd;
    std::mt19937 m_mt;
    std::random_device::result_type m_seed;
};

/**
 * @brief   Convenience accessor to the global `rng` instance
 *
 * Each thread will get its own instance.
 */
inline rng& random() {
    static thread_local rng obj;
    return obj;
}

/**
 * @brief   Convenience accessor to the global `rng` instance
 *
 * Each thread will get its own instance.
 *
 * @param seed
 */
inline rng& random(std::random_device::result_type seed) {
    static thread_local rng obj{ seed };
    return obj;
}

} // namespace nova
