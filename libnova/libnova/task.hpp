/**
 * Part of Nova C++ Library.
 *
 * @brief   A unit of work from a scheduling perspective for an executor.
 *
 * A move-only callable without arguments or result.
 *
 * Move-only, unlike `std::function`, so a task can own what it works on (a `unique_ptr`, a
 * promise).
 *
 * TODO: `std::move_only_function` (C++23) replaces it once Nova requires C++23.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-10-22
 * @updated     2026-10-22
 * @status      Concept
 *
 * @compliance  Under evaluation
 */

#pragma once

#include <concepts>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace nova {

class task {
public:
    task() = default;

    template <typename F>
        requires std::invocable<std::decay_t<F>&>
            and (not std::same_as<std::remove_cvref_t<F>, task>)
    task(F&& func)                                                                                  // NOLINT(google-explicit-constructor) | Any callable is a task, like `std::function`
        : m_impl(std::make_unique<model<std::decay_t<F>>>(std::forward<F>(func)))
    {}

    task(task&&) noexcept = default;
    auto operator=(task&&) noexcept -> task& = default;

    task(const task&) = delete;
    auto operator=(const task&) -> task& = delete;

    ~task() = default;

    void operator()() {
        m_impl->call();
    }

    [[nodiscard]] explicit operator bool() const {
        return static_cast<bool>(m_impl);
    }

private:
    struct callable {
        virtual void call() = 0;
        virtual ~callable() = default;
    };

    template <typename F>
    struct model final : callable {
        explicit model(F f) : func(std::move(f)) {}
        void call() override { std::invoke(func); }
        F func;
    };

    std::unique_ptr<callable> m_impl;

};

} // namespace nova
