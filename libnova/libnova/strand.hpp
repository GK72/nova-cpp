/**
 * Part of Nova C++ Library.
 *
 * @brief   A logical thread. A strand sequences the tasks while allowing the
 *          executor to schedule it automatically on available threads.
 *
 * The order is guaranteed to be the same as the tasks were posted on the
 * strand.
 *
 * A run of the strand on the executor takes the tasks that were waiting when
 * it started in batches. It's not guaranteed that the complete batch is
 * processed, but if the strand is destroyed, it waits for the currently running
 * task to finish.
 *
 * The tasks posted meanwhile wait for the next run, at the end of
 * the executor's queue, so a busy strand does not keep a thread from others.
 *
 * The actor must not use global (nor thread local) variables/objects (unless
 * they are protected with mutexes of course).
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

#include <memory>

namespace nova {

namespace detail {
    struct strand_state;
} // namespace detail

/**
 * @brief   A logical thread.
 *
 * Its state is shared with the scheduler.
 */
class strand : public executor {
public:
    /**
     * @brief   A strand on `target`, which must outlive it.
     */
    explicit strand(executor& target);

    /**
     * @brief   Drop the tasks not started yet and wait for the running one.
     *
     * Destroying a strand from one of its own tasks does not wait; it is
     * reported to the log.
     */
    ~strand() override;

    strand(const strand&) = delete;
    strand(strand&&) = delete;
    auto operator=(const strand&) -> strand& = delete;
    auto operator=(strand&&) -> strand& = delete;

    /**
     * @brief   Queue work; it runs after the tasks posted before it.
     *
     * A task posted while the strand is destroyed is dropped. When the
     * executor cannot take the run of the strand, the exception passes on and
     * the task stays queued for the run of the next post.
     */
    void post(task work) override;

private:
    std::shared_ptr<detail::strand_state> m_state;

};

} // namespace nova
