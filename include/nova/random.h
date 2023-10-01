#pragma once

#include "nova/types.h"

#include <concepts>
#include <numeric>
#include <random>
#include <ranges>
#include <type_traits>

namespace nova {

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

/**
 * @brief   A convenience class to generate random things
 *
 * Contains the engine and seed with some wrapper member functions.
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
        return pick_from(r, m_mt);
    }

    /**
     * @brief   Generate a number in the given `range`
     */
    template <typename T>
    auto number(range<T> r) -> T {
        return generate_number(r, m_mt);
    }

    /**
     * @brief   Generate a number in the range of [0, 1)
     */
    auto number() -> float {
        return generate_number(range<float>{ 0.0F, 1.0F }, m_mt);
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

} // namespace nova
