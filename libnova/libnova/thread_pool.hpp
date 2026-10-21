/**
 * Part of Nova C++ Library.
 *
 * @brief   A fixed set of threads running posted tasks in the order they were posted.
 *
 * For work that may block or take long (loading, I/O, a language model answering); frame work
 * with a deadline needs a job system with dependencies, which this is not.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-10-22
 * @updated     2026-10-22
 * @status      Concept
 *
 * @compliance  Under evaluation
 */

#pragma once

#include <libnova/executor.hpp>
#include <libnova/task.hpp>

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace nova {

struct pool_config {
    std::string name;               ///< Names the threads, e.g., in a debugger or the profiler
    std::size_t threads = 1;
};

class thread_pool : public executor {
public:
    explicit thread_pool(pool_config config);

    /**
     * @brief   Run the tasks already posted, then join the threads.
     */
    ~thread_pool() override;

    thread_pool(const thread_pool&) = delete;
    thread_pool(thread_pool&&) = delete;
    auto operator=(const thread_pool&) -> thread_pool& = delete;
    auto operator=(thread_pool&&) -> thread_pool& = delete;

    void post(task work) override;

    [[nodiscard]] auto size() const -> std::size_t { return m_threads.size(); }

private:
    std::string m_name;
    std::mutex m_mutex;
    std::condition_variable m_ready;
    std::deque<task> m_queue;
    bool m_stopping = false;
    std::vector<std::jthread> m_threads;    ///< Last: started after everything they use

    void run(std::size_t index);

};

/**
 * @brief   The process's pool for background work: a quarter of the hardware threads, at least
 *          two.
 *
 * Process-wide, so it is not budgeted with any other pool: an application that budgets its
 * CPU threads owns its pools and hands them out instead.
 */
[[nodiscard]] auto background() -> executor&;

} // namespace nova
