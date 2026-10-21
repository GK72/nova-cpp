/**
 * Part of Nova C++ Library Examples.
 *
 * Executors.
 *
 * An event loop keeps its pace while slow work runs on a thread pool; the work reports back to
 * the loop as results in a queue, not as tasks run on it.
 * - A strand serialises the calls of a slow, stateful, not thread-safe producer
 * - Loading runs in parallel, its results come back as futures: a task owns its promise
 * - A failing task is reported to the log; the pool keeps running
 * - Work logs where it runs, so the thread and time of each line show where it happened
 *
 * `executors --inline` runs the same code on the inline executor, on one thread: the producer's
 * work then stalls the loop, which the worst lateness of a tick shows.
 */

#include <libnova/executor.hpp>
#include <libnova/log.hpp>
#include <libnova/main.hpp>
#include <libnova/random.hpp>
#include <libnova/strand.hpp>
#include <libnova/thread_pool.hpp>
#include <libnova/types.hpp>
#include <libnova/utils.hpp>

#include <fmt/chrono.h>
#include <fmt/format.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <future>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

namespace {

struct result {
    int id;
    std::string text;
};

/**
 * @brief   Results for the loop from any thread; the loop takes all that arrived at once.
 */
class queue {
public:
    void push(result r) {
        const auto lock = std::scoped_lock(m_mutex);
        m_results.push_back(std::move(r));
    }

    /**
     * @brief   Swap out everything that arrived: one lock per batch, not per result.
     */
    void drain(std::vector<result>& out) {
        out.clear();
        const auto lock = std::scoped_lock(m_mutex);
        std::swap(out, m_results);
    }

private:
    std::mutex m_mutex;
    std::vector<result> m_results;

};

void simulate_work() {
    std::this_thread::sleep_for(20ms);
}

/**
 * @brief   An example for a slow, stateful producer.
 *
 * Its work goes through the strand, so it never overlaps and its state needs no lock.
 */
class stateful_async_producer {
public:

    /**
     * @brief   Create producer binding it to the executor and attaching output
     *          for the results.
     */
    stateful_async_producer(nova::executor& executor, queue& out)
        : m_out(out)
        , m_strand(executor)
    {}

    /**
     * @brief   Produce a result which arrives in the output queue.
     */
    void produce(int id) {
        m_strand.post([this, id]() {
            simulate_work();
            ++m_done;

            nova::log::info("Produced #{}", m_done);
            m_out.push(result{ id, fmt::format("result #{}", m_done) });
        });
    }

private:
    queue& m_out;
    int m_done = 0;         ///< Unguarded: only the tasks of the strand touch it
    nova::rng m_rng;        ///< Unguarded: only the tasks of the strand touch it
    nova::strand m_strand;  ///< Last: destroyed first, waiting for the task using the others

};

/**
 * @brief   Do some work in parallel and wait for them all.
 *
 * The work is submitted onto the executor which returns futures.
 *
 * A missing one fails illustrating the error handling: `std::future` swallows
 * the exception and rethrows it upon accessing the result.
 *
 * Each load logs when it is done, on its own thread; the futures only join
 * them.
 */
void load_assets(nova::executor& executor) {
    const auto names = std::vector<std::string>{ "asset-1", "asset-2", "missing-asset-3", "asset-4" };
    auto results = std::vector<std::future<int>>{};

    for (const auto& name : names) {
        auto work = [name]() -> int {
            simulate_work();

            if (name.starts_with("missing")) {
                throw std::runtime_error(fmt::format("{} not found", name));
            }

            nova::log::info("Loaded {}", name);
            return std::stoi(name.substr(name.size() - 2));
        };

        results.push_back(nova::submit(executor, work));
    }

    auto loaded = std::size_t{ 0 };
    for (std::size_t i = 0; i < names.size(); ++i) {
        try {
            results[i].get();
            ++loaded;
        }
        catch (const std::exception& err) {
            nova::log::warn("Loading {} failed: {}", names[i], err.what());
        }
    }

    nova::log::info("Loaded {} of {} assets", loaded, names.size());
}

/**
 * @brief   Run 200 ticks of 10 ms: ask the producer now and then, and take in its results.
 *
 * TODO: This should also be part of the framework, shouldn't it?
 */
void run_loop(stateful_async_producer& producer, queue& input_queue) {
    constexpr auto tick = 10ms;
    constexpr auto ticks = 200;

    auto batch = std::vector<result>{};
    auto id = 0;
    auto received = 0;
    auto worst_late = std::chrono::nanoseconds{ 0 };

    auto due = std::chrono::steady_clock::now();
    for (int i = 0; i < ticks; ++i) {
        worst_late = std::max(worst_late, std::chrono::steady_clock::now() - due);

        if (i % 20 == 0) {
            producer.produce(id);
            ++id;
        }

        input_queue.drain(batch);
        for (const auto& r : batch) {
            ++received;
            nova::log::info("Received {}", r.text);
        }

        due += tick;
        std::this_thread::sleep_until(due);
    }

    nova::log::info("Produced: {}, received: {}", id, received);
    nova::log::info("Worst lateness of a tick: {}", std::chrono::duration_cast<std::chrono::microseconds>(worst_late));
}

} // namespace

auto entrypoint(auto args) -> int {
    nova::log::init("executors");

    const auto use_inline = std::ranges::any_of(args, [](auto arg) { return arg == "--inline"; });

    auto pool = nova::thread_pool({ .name = "example", .threads = 4 });
    auto inline_ex = nova::inline_executor{};
    auto& executor = use_inline ? static_cast<nova::executor&>(inline_ex) : pool;

    nova::log::info("Running on the {}", use_inline ? "inline executor" : "thread pool");

    load_assets(executor);

    executor.post([]() { throw std::runtime_error("expected in the example"); });

    auto output_queue = queue{};
    auto producer = stateful_async_producer(executor, output_queue);

    auto stopwatch = nova::stopwatch();
    run_loop(producer, output_queue);
    nova::log::info("Loop ran for {}", std::chrono::duration_cast<std::chrono::milliseconds>(stopwatch.elapsed()));

    return EXIT_SUCCESS;
}

NOVA_MAIN(entrypoint);
