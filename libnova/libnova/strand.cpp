/**
 * Part of Nova C++ Library.
 *
 * @brief   Strand.
 *
 * At most one run of the strand is scheduled on the target at a time
 * (`scheduled`); it runs the tasks that were waiting when it started and
 * schedules the next run if more arrived meanwhile.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-10-22
 * @updated     2026-10-22
 * @status      Concept
 *
 * @compliance  Under evaluation
 */

#include <libnova/strand.hpp>

#include <libnova/log.hpp>

#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>

namespace nova {

namespace detail {

struct strand_state {
    executor& target;
    std::condition_variable idle;

    std::mutex mutex;
    std::deque<task> queue;
    bool scheduled = false;                 ///< A run is posted to the target or running
    bool closed = false;                    ///< The strand is destroyed; nothing runs any more
    std::optional<std::thread::id> running; ///< Thread of the task being run

    explicit strand_state(executor& t) : target(t) {}
};

} // namespace detail

namespace {

    void schedule(const std::shared_ptr<detail::strand_state>& s);

    /**
     * @brief   Run the tasks that were waiting when the run started, then
     *          schedule the next run if more arrived meanwhile.
     *
     * Tasks posted during the run wait for the next one, which goes to the
     * end of the target's queue: a busy strand neither keeps a thread from
     * the others nor pays a trip through the target's queue for every task.
     *
     * A task is destroyed before the strand counts as idle: its captures may
     * refer to the strand's owner, which the destructor's wait protects only
     * while a task is running.
     *
     * @throws  the task's exception if any.
     */
    void run_next(const std::shared_ptr<detail::strand_state>& s) {
        auto lock = std::unique_lock(s->mutex);

        for (auto batch = s->queue.size(); batch > 0 and not s->closed; --batch) {
            auto work = std::move(s->queue.front());
            s->queue.pop_front();
            s->running = std::this_thread::get_id();
            lock.unlock();

            run_reported(work, "a strand");
            work = task{};

            lock.lock();
            s->running.reset();
            s->idle.notify_all();
        }

        if (s->closed or s->queue.empty()) {
            s->scheduled = false;
            return;
        }

        lock.unlock();
        schedule(s);
    }

    /**
     * @brief   Post the next run of the strand to its target.
     *
     * When the target cannot take it, the strand is not scheduled any more,
     * so the next post schedules a run again instead of the strand stalling.
     */
    void schedule(const std::shared_ptr<detail::strand_state>& s) {
        try {
            s->target.post([s]() { run_next(s); });
        }
        catch (...) {
            const auto lock = std::scoped_lock(s->mutex);
            s->scheduled = false;
            throw;
        }
    }

} // namespace

strand::strand(executor& target)
    : m_state(std::make_shared<detail::strand_state>(target))
{}

strand::~strand() {
    auto dropped = std::deque<task>{};
    auto lock = std::unique_lock(m_state->mutex);

    m_state->closed = true;
    std::swap(dropped, m_state->queue);

    if (m_state->running == std::this_thread::get_id()) {
        nova::topic_log::warn("exec", "A strand is destroyed by its own task; not waiting for it");
        return;
    }

    m_state->idle.wait(lock, [this]() { return not m_state->running.has_value(); });
}

/**
 * @note    Only a post to an idle strand schedules a run on the target.
 *
 * Later posts join the queue that run drains. They will be automaticall
 * scheduled.
 *
 * A strand being destroyed is closed: a task posted to it, by its running
 * task or by a dropped one being destroyed, is dropped too, outside the lock.
 */
void strand::post(task work) {
    auto closed = false;

    {
        const auto lock = std::scoped_lock(m_state->mutex);
        closed = m_state->closed;

        if (not closed) {
            m_state->queue.push_back(std::move(work));

            if (m_state->scheduled) {
                return;
            }

            m_state->scheduled = true;
        }
    }

    if (closed) {
        nova::topic_log::warn("exec", "A task is posted to a strand being destroyed; dropping it");
        return;
    }

    schedule(m_state);
}

} // namespace nova
