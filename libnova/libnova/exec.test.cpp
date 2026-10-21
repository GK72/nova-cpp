#include <libnova/executor.hpp>
#include <libnova/strand.hpp>
#include <libnova/thread_pool.hpp>

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <future>
#include <latch>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

namespace {

/**
 * @brief   Throws on its first post and runs the later ones at once.
 */
class failing_once_executor : public nova::executor {
public:
    void post(nova::task work) override {
        if (not m_failed) {
            m_failed = true;
            throw std::runtime_error("expected in the test");
        }

        nova::run_reported(work, "the test");
    }

private:
    bool m_failed = false;

};

} // namespace

TEST(Task, MoveOnlyCallable) {
    auto value = std::make_unique<int>(41);
    auto result = 0;

    auto work = nova::task([&result, value = std::move(value)]() { result = *value + 1; });
    auto moved = std::move(work);

    ASSERT_TRUE(static_cast<bool>(moved));
    moved();
    EXPECT_EQ(result, 42);
}

TEST(InlineExecutor, RunsAtOnce) {
    auto ex = nova::inline_executor{};
    auto ran = false;

    ex.post([&ran]() { ran = true; });

    EXPECT_TRUE(ran);
}

TEST(Submit, DeliversTheResult) {
    auto pool = nova::thread_pool({ .name = "test", .threads = 1 });
    auto value = std::make_unique<int>(41);

    auto result = nova::submit(pool, [value = std::move(value)]() { return *value + 1; });

    EXPECT_EQ(result.get(), 42);
}

TEST(Submit, DeliversTheException) {
    auto ex = nova::inline_executor{};

    auto result = nova::submit(ex, []() { throw std::runtime_error("expected in the test"); });

    EXPECT_THROW(result.get(), std::runtime_error);
}

TEST(ThreadPool, RunsEveryTask) {
    auto count = std::atomic<int>{ 0 };

    {
        auto pool = nova::thread_pool({ .name = "test", .threads = 4 });
        for (int i = 0; i < 1000; ++i) {
            pool.post([&count]() { ++count; });
        }
    }   // Runs what was posted, then joins

    EXPECT_EQ(count, 1000);
}

TEST(ThreadPool, RunsInParallel) {
    auto pool = nova::thread_pool({ .name = "test", .threads = 4 });
    auto all_started = std::latch(5);

    for (int i = 0; i < 4; ++i) {
        pool.post([&all_started]() { all_started.arrive_and_wait(); });   // Deadlocks unless 4 run at once
    }

    all_started.arrive_and_wait();
}

TEST(ThreadPool, FailingTaskDoesNotStopIt) {
    auto pool = nova::thread_pool({ .name = "test", .threads = 1 });
    auto done = std::promise<void>{};

    pool.post([]() { throw std::runtime_error("expected in the test"); });
    pool.post([&done]() { done.set_value(); });

    EXPECT_EQ(done.get_future().wait_for(5s), std::future_status::ready);
}

TEST(ThreadPool, TasksPostedWhileStoppingRun) {
    auto count = std::atomic<int>{ 0 };

    {
        auto pool = nova::thread_pool({ .name = "test", .threads = 2 });
        pool.post([&pool, &count]() {
            std::this_thread::sleep_for(10ms);
            pool.post([&count]() { ++count; });
        });
    }

    EXPECT_EQ(count, 1);
}

TEST(Strand, InOrderAndNeverOverlapping) {
    auto pool = nova::thread_pool({ .name = "test", .threads = 8 });
    auto order = std::vector<int>{};
    auto active = std::atomic<int>{ 0 };
    auto overlapped = std::atomic<bool>{ false };
    auto done = std::promise<void>{};

    {
        auto s = nova::strand(pool);

        for (int i = 0; i < 500; ++i) {
            s.post([&, i]() {
                if (++active > 1) {
                    overlapped = true;
                }

                order.push_back(i);     // Unsynchronized on purpose: the strand serializes
                --active;

                if (i == 499) {
                    done.set_value();
                }
            });
        }

        ASSERT_EQ(done.get_future().wait_for(5s), std::future_status::ready);
    }

    EXPECT_FALSE(overlapped);
    ASSERT_EQ(order.size(), 500U);
    for (int i = 0; i < 500; ++i) {
        EXPECT_EQ(order[static_cast<std::size_t>(i)], i);
    }
}

TEST(Strand, StrandsShareThePool) {
    auto pool = nova::thread_pool({ .name = "test", .threads = 2 });
    auto a = nova::strand(pool);
    auto b = nova::strand(pool);
    auto both = std::latch(2);

    a.post([&both]() { both.arrive_and_wait(); });     // Deadlocks unless the strands run at once
    b.post([&both]() { both.arrive_and_wait(); });

    both.wait();
}

TEST(Strand, DestructionDropsQueuedAndWaitsForRunning) {
    auto pool = nova::thread_pool({ .name = "test", .threads = 2 });
    auto started = std::promise<void>{};
    auto finished = std::atomic<bool>{ false };
    auto dropped_ran = std::atomic<bool>{ false };

    {
        auto s = nova::strand(pool);

        s.post([&]() {
            started.set_value();
            std::this_thread::sleep_for(50ms);
            finished = true;
        });
        s.post([&]() { dropped_ran = true; });

        started.get_future().wait();
    }   // Waits for the first task

    EXPECT_TRUE(finished);
    std::this_thread::sleep_for(20ms);
    EXPECT_FALSE(dropped_ran);
}

TEST(Strand, DestroyedByItsOwnTask) {
    auto pool = nova::thread_pool({ .name = "test", .threads = 1 });
    auto s = std::make_unique<nova::strand>(pool);
    auto done = std::promise<void>{};

    s->post([&]() {
        s.reset();
        done.set_value();
    });

    EXPECT_EQ(done.get_future().wait_for(5s), std::future_status::ready);
}

TEST(Strand, PostedWhileDestroyedIsDropped) {
    auto pool = nova::thread_pool({ .name = "test", .threads = 1 });
    auto started = std::promise<void>{};
    auto ran = std::atomic<bool>{ false };
    auto destroyed = std::atomic<bool>{ false };

    {
        auto s = nova::strand(pool);

        s.post([&]() {
            started.set_value();
            std::this_thread::sleep_for(50ms);      // The destructor closes the strand meanwhile

            auto guard = std::shared_ptr<void>(nullptr, [&destroyed](void*) { destroyed = true; });
            s.post([&ran, guard]() { ran = true; });
        });

        started.get_future().wait();
    }   // Waits for the task, which posts to the closed strand

    EXPECT_TRUE(destroyed);     // Dropped at once, not left in the queue of the shared state
    EXPECT_FALSE(ran);
}

TEST(Strand, RecoversWhenTheExecutorThrows) {
    auto ex = failing_once_executor{};
    auto s = nova::strand(ex);
    auto order = std::vector<int>{};

    EXPECT_THROW(s.post([&]() { order.push_back(1); }), std::runtime_error);
    s.post([&]() { order.push_back(2); });      // Schedules again, running the queued one first

    EXPECT_EQ(order, (std::vector<int>{ 1, 2 }));
}

TEST(Strand, RunsTheWaitingTasksThenYields) {
    auto pool = nova::thread_pool({ .name = "test", .threads = 1 });
    auto gate = std::promise<void>{};
    auto order = std::vector<std::string>{};    // Only the one thread of the pool touches it
    auto done = std::promise<void>{};

    pool.post([future = gate.get_future()]() { future.wait(); });  // Holds the thread until all is queued

    {
        auto s = nova::strand(pool);

        s.post([&]() {
            order.emplace_back("a1");
            s.post([&]() {                      // Posted during the run: waits for the next one
                order.emplace_back("a3");
                done.set_value();
            });
        });
        s.post([&]() { order.emplace_back("a2"); });
        pool.post([&]() { order.emplace_back("other"); });

        gate.set_value();
        ASSERT_EQ(done.get_future().wait_for(5s), std::future_status::ready);
    }

    EXPECT_EQ(order, (std::vector<std::string>{ "a1", "a2", "other", "a3" }));
}

TEST(Strand, OnTheInlineExecutor) {
    auto ex = nova::inline_executor{};
    auto s = nova::strand(ex);
    auto order = std::vector<int>{};

    s.post([&]() {
        order.push_back(1);
        s.post([&]() { order.push_back(3); });      // Runs after this task, not inside it
        order.push_back(2);
    });

    EXPECT_EQ(order, (std::vector<int>{ 1, 2, 3 }));
}

TEST(Background, IsAPool) {
    auto done = std::promise<void>{};
    nova::background().post([&done]() { done.set_value(); });

    EXPECT_EQ(done.get_future().wait_for(5s), std::future_status::ready);
}
