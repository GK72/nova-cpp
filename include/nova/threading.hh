/**
 * Part of Nova C++ Library.
 *
 * Everything that concerns the execution.
 * - Thread handling, thread pools (TODO)
 * - Task management (TODO)
 * - Timings, event loops
 */

#pragma once

#include "nova/intrinsics.hh"
#include "nova/utils.hh"

#include <chrono>

namespace nova {

struct timings {
    std::chrono::nanoseconds interval;
    std::chrono::nanoseconds limit;
};

template <typename Callable>
class event_loop {
public:
    event_loop(Callable& func, timings timings_)
        : m_func(func)
        , m_timings(timings_)
    {}

    /**
     * @brief   Loop until the set limit.
     *
     * The callable is called when the delta time reaches the interval
     * and not before.
     *
     * Delta measures the time between the calls.
     */
    void start() {
        using namespace std::chrono_literals;

        while (m_total_elapsed < m_timings.limit) {
            m_delta += m_stopwatch.elapsed();
            if (m_delta > m_timings.interval) {
                std::invoke(m_func, m_delta, nova::rdtsc());
                m_total_elapsed += m_delta;
                m_delta = 0ns;
            }
        }
    }

private:
    Callable& m_func;
    timings m_timings;
    stopwatch m_stopwatch {};
    std::chrono::nanoseconds m_total_elapsed {};
    std::chrono::nanoseconds m_delta {};

};

} // namespace nova
