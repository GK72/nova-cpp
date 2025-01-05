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
 *
 * Capability must be enabled: `sudo setcap cap_sys_nice+ep shielded`
 */

#include <nova/log.hh>
#include <nova/main.hh>
#include <nova/system.hh>
#include <nova/threading.hh>

#include <fmt/chrono.h>
#include <fmt/format.h>

#include <algorithm>
#include <cstdint>
#include <chrono>
#include <numeric>
#include <string>
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
        m_cycles.reserve(10000);
    }

    void operator()(std::chrono::nanoseconds delta, std::uint64_t cycles) {
        nova::log::debug("Delta: {}, Cycles: {}", delta, cycles);
        m_measurements.push_back(delta);
        m_cycles.push_back(cycles);
    }

    const auto& measurements() const { return m_measurements; }
    const auto& cycles()       const { return m_cycles; }

private:
    std::vector<std::chrono::nanoseconds> m_measurements;
    std::vector<std::uint64_t> m_cycles;
};

[[nodiscard]] auto parse_args(auto args) -> nova::timings {
    if (args.size() < 3) {
        return {
            .interval = 100ms,
            .limit = 1s
        };
    }

    return {
        .interval = std::chrono::microseconds{ std::stoll(std::string(args[1])) },
        .limit    = std::chrono::microseconds{ std::stoll(std::string(args[2])) }
    };
}

/**
 * @brief   Run the loop for a little while and log some statistics.
 */
auto entrypoint([[maybe_unused]] auto args) -> int {
    auto& logger = nova::log::init("shielded");
    logger.set_level(spdlog::level::debug);

    const auto timings = parse_args(args);

    const auto cfg = nova::process_scheduling{
        .pid = nova::get_pid(),
        .cpu = 0,
        .priority = nova::process_priority::critical
    };

    if (const auto result = nova::set_cpu_affinity(cfg); not result.has_value()) {
        nova::log::warn("{}", result.error().message);
    }

    auto logic = event_loop_impl{};
    auto loop = nova::event_loop(logic, timings);

    auto stopwatch = nova::stopwatch();
    loop.start();

    auto xs = logic.measurements();
    if (xs.empty()) {
        return EXIT_FAILURE;
    }

    std::ranges::sort(xs);

    nova::log::info("Min delta: {}", std::ranges::min(xs));
    nova::log::info("Max delta: {}", std::ranges::max(xs));
    nova::log::info("Avg delta: {}", std::accumulate(std::begin(xs), std::end(xs), 0ns) / xs.size());
    nova::log::info("Med delta: {}", xs.at(xs.size() / 2));
    nova::log::info("Size: {}", xs.size());

    nova::log::info("Total elapsed time: {}", stopwatch.elapsed());

    return EXIT_SUCCESS;
}

MAIN(entrypoint);
