/**
 * Part of Nova C++ Library.
 *
 * Thread pool, inline executor and the reporting of failed tasks.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-10-22
 * @updated     2026-10-22
 * @status      Concept
 *
 * @compliance  Under evaluation
 */

#include <libnova/thread_pool.hpp>

#include <libnova/log.hpp>

#include <fmt/format.h>

#include <algorithm>
#include <exception>
#include <utility>

#ifdef __linux__
#include <pthread.h>
#endif

namespace nova {

namespace {

    /**
     * @brief   Name the calling thread; Linux keeps 15 characters.
     */
    void name_thread([[maybe_unused]] const std::string& name) {
#ifdef __linux__
        pthread_setname_np(pthread_self(), name.substr(0, 15).c_str());
#endif
    }

} // namespace

void run_reported(task& work, const char* where) noexcept {
    try {
        work();
    }
    catch (const std::exception& ex) {
        nova::topic_log::error("exec", "A task of {} failed: {}", where, ex.what());
    }
    catch (...) {
        nova::topic_log::error("exec", "A task of {} failed with an unknown exception", where);
    }
}

void inline_executor::post(task work) {
    run_reported(work, "the inline executor");
}

thread_pool::thread_pool(pool_config config)
    : m_name(std::move(config.name))
{
    m_threads.reserve(config.threads);

    for (std::size_t i = 0; i < config.threads; ++i) {
        m_threads.emplace_back([this, i]() { run(i); });
    }
}

thread_pool::~thread_pool() {
    {
        const auto lock = std::scoped_lock(m_mutex);
        m_stopping = true;
    }

    m_ready.notify_all();
    m_threads.clear();      // Joins
}

void thread_pool::post(task work) {
    {
        const auto lock = std::scoped_lock(m_mutex);
        m_queue.push_back(std::move(work));
    }

    m_ready.notify_one();
}

/**
 * A worker leaves when the pool stops and the queue is empty, so tasks posted while stopping,
 * also by other tasks, still run.
 */
void thread_pool::run(std::size_t index) {
    name_thread(fmt::format("{}-{}", m_name, index));

    while (true) {
        auto work = task{};

        {
            auto lock = std::unique_lock(m_mutex);
            m_ready.wait(lock, [this]() { return m_stopping or not m_queue.empty(); });

            if (m_queue.empty()) {
                return;
            }

            work = std::move(m_queue.front());
            m_queue.pop_front();
        }

        run_reported(work, m_name.c_str());
    }
}

auto background() -> executor& {
    static auto pool = thread_pool({
        .name = "background",
        .threads = std::max(2U, std::thread::hardware_concurrency() / 4),
    });

    return pool;
}

} // namespace nova
