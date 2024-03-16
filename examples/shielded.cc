/**
 * Part of Nova C++ Library Examples.
 *
 * Shielded CPU.
 *
 * TODO(feat): Forbid other processes to use the shielded CPU.
 *             Current implementation only sets the affinity and
 *             makes the process high priority.
 *
 * https://www.linuxjournal.com/article/6900
 */

#include "nova/main.h"
#include "nova/system.h"
#include "nova/threading.h"
#include "nova/utils.h"

#include <fmt/chrono.h>
#include <fmt/format.h>

#include <algorithm>
#include <chrono>
#include <numeric>
#include <vector>

using namespace std::chrono_literals;

/**
 * @brief   An example function object that can be called by the event loop.
 *
 * Also serves as benchmarking.
 */
class event_loop_impl {
public:
    event_loop_impl() {
        m_measurements.reserve(10000);
    }

    void operator()(std::chrono::nanoseconds delta) {
        m_measurements.push_back(delta);
    }

    const auto& measurements() const { return m_measurements; }

private:
    std::vector<std::chrono::nanoseconds> m_measurements;
};

/**
 * @brief   Run the loop for a little while and log some statistics.
 */
auto entrypoint([[maybe_unused]] auto args) -> int {
    auto& logger = nova::log_init("Shielded");
    logger.set_level(spdlog::level::debug);

    const auto timings = nova::timings{
        .interval = 1ms,
        .limit = 1s
    };

    const auto cfg = nova::process_scheduling{
        .pid = nova::get_pid(),
        .cpu = 0,
        .priority = nova::process_priority::critical
    };

    nova::set_cpu_affinity(cfg).value();

    auto logic = event_loop_impl{};
    auto loop = nova::event_loop(logic, timings);
    loop.start();

    auto xs = logic.measurements();
    if (xs.empty()) {
        return EXIT_FAILURE;
    }

    std::ranges::sort(xs);

    spdlog::info("Min delta: {}", std::ranges::min(xs));
    spdlog::info("Max delta: {}", std::ranges::max(xs));
    spdlog::info("Avg delta: {}", std::accumulate(std::begin(xs), std::end(xs), 0ns) / xs.size());
    spdlog::info("Med delta: {}", xs.at(xs.size() / 2));
    spdlog::info("Size: {}", xs.size());

    return EXIT_SUCCESS;
}

MAIN(entrypoint);
