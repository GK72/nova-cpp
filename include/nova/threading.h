#pragma once

#include "types.h"

#include <atomic>
#include <chrono>
#include <concepts>
#include <csignal>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>

#if defined (__unix__)
    #include <pthread.h>
#endif

namespace nova {

namespace detail {
    // NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables) | Hidden in `detail` namespace, used only by signal_handler
    inline std::atomic_int g_signal { 0 };
    inline std::atomic_int g_num_signals { 0 };
    // NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

    inline void sig_handler(int signal) {
        g_signal.store(signal);
        ++g_num_signals;
    }

    struct default_context_callable {
        bool operator()([[maybe_unused]] context& _) { return true; }
    };
} // namespace detail

template <typename Callable>
    requires std::invocable<
        Callable,
        const std::atomic_int&,
        const std::atomic_int&,
        bool
    >
class signal_handler {
public:
    signal_handler(Callable&& func)
        : m_func(std::forward<Callable>(func))
    {
        if (std::signal(SIGINT, detail::sig_handler) == SIG_ERR) {
            m_disabled = true;
        }
    }

private:
    Callable m_func;
    bool m_disabled = false;
};

class thread {
private:
    static constexpr auto MaxNameLength = 15;

public:
    template <typename Callable>
    thread(const std::string& name, context& ctx, Callable&& func)
        : m_name(set_name(name))
        , m_thread([&ctx, func = std::forward<Callable>(func)]{
            try {
                func();
            } catch (const std::exception&) {
                auto lock = std::lock_guard(ctx.mtx);
                ctx.exception = std::current_exception();
            }
        })
    {
        p_set_name(name);
    }

    [[nodiscard]] auto name() const { return m_name; }

    void join() {
        m_thread.join();
    }

private:
    std::string m_name;
    std::jthread m_thread;

    std::string set_name(const std::string& name) {
        if (name.size() > MaxNameLength) {
            throw std::invalid_argument("The name of the thread cannot be larger than 16 bytes (including null terminator).");
        }
        return name;
    }

    void p_set_name(const std::string& name) {
        if (not name.empty()) {
            #if defined (__unix__)
                pthread_setname_np(m_thread.native_handle(), name.c_str());
            #else
                #warning "Only Linux is supported for the thread set name magick!"
            #endif
        }
    }
};

template <typename Callable = detail::default_context_callable>
    requires std::is_invocable_r_v<bool, Callable, context&>
void daemon(const std::chrono::microseconds& interval, context& ctx, Callable watcher = {}) {
    constexpr auto stop = [](auto& ctx_) {
        ctx_.keep_alive.store(false);
        ctx_.cv.notify_all();
    };

    const auto handle_signal = signal_handler {
        [&ctx, stop](const std::atomic_int& signal, [[maybe_unused]] const std::atomic_int& num_signals, bool disabled) {
            if (disabled) {
                // Signal handler is disabled
                return;
            }
            if (signal.load() == SIGINT) {
                // Interrupted - Signal received: {}
                stop(ctx);
            }
        }
    };

    while (ctx.keep_alive) {
        handle_signal();

        const auto copy_exception_ptr = [&ctx] {
            auto lock = std::lock_guard(ctx.mtx);
            return ctx.exception;
        };

        const auto ex_ptr = copy_exception_ptr();
        if (ex_ptr) {
            stop(ctx);
            std::rethrow_exception(ex_ptr);
        }

        std::this_thread::sleep_for(interval);

        if (not watcher(ctx)) {
            stop(ctx);
        }
    }
}

} // namespace nova
