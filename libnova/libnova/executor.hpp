/**
 * Part of Nova C++ Library.
 *
 * @brief   Executors: where and when tasks run, separate from what they do.
 *
 * Code that has work to do takes an `executor&` and posts tasks to it; the owner of the program
 * decides whether that is a thread pool, a strand on one, or the caller's own thread. Shaped
 * like the schedulers of `std::execution` (C++26), so moving to them later changes the
 * implementations, not the callers.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-10-22
 * @updated     2026-10-22
 * @status      Concept
 *
 * @compliance  Under evaluation
 */

#pragma once

#include <libnova/task.hpp>

#include <concepts>
#include <exception>
#include <future>
#include <type_traits>
#include <utility>

namespace nova {

class executor {
public:
    /**
     * @brief   Run the task later, on a thread of the executor; it returns at once.
     *
     * An exception that leaves a task is reported to the log and does not stop the executor.
     */
    virtual void post(task work) = 0;

    virtual ~executor() = default;
};

/**
 * @brief   Runs every task at once, on the posting thread; for tests and single-threaded tools.
 */
class inline_executor : public executor {
public:
    void post(task work) override;
};

/**
 * @brief   Run `work`, reporting an exception that leaves it instead of passing it on.
 */
void run_reported(task& work, const char* where) noexcept;

/**
 * @brief   Run `func` on `ex`, its result or exception arriving in the future.
 *
 * Results are layered on top of tasks, so executors know nothing about them; another way of
 * delivering a result, e.g., a callback, is another free function like this one.
 */
template <typename F>
    requires std::invocable<F&>
auto submit(executor& ex, F func) -> std::future<std::invoke_result_t<F&>> {
    using result_type = std::invoke_result_t<F&>;

    auto promise = std::promise<result_type>{};
    auto future = promise.get_future();

    ex.post([func = std::move(func), promise = std::move(promise)]() mutable {
        try {
            if constexpr (std::is_void_v<result_type>) {
                func();
                promise.set_value();
            }
            else {
                promise.set_value(func());
            }
        }
        catch (...) {
            promise.set_exception(std::current_exception());
        }
    });

    return future;
}

} // namespace nova
