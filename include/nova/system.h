/**
 * Part of Nova C++ Library.
 *
 * Operating System specific functionalities.
 *
 * Some function may seem like useless, but they are for avoiding direct
 * platform specific calls.
 */

#pragma once

#include "nova/error.h"
#include "nova/intrinsics.h"

#include <utility>

#ifdef NOVA_LINUX
#include <sched.h>
#include <unistd.h>
#include <sys/resource.h>
#else
#error "The target platform does not support OS specific code"
#endif

namespace nova {

enum class process_priority {
    critical = -20
};

struct process_scheduling {
    int pid;
    std::size_t cpu;
    process_priority priority;
};

[[nodiscard]] inline auto get_pid() -> int {
    return getpid();
}

/**
 * @brief   Set CPU affinity and process priority.
 */
[[nodiscard]] inline
auto set_cpu_affinity(const process_scheduling& cfg) -> expected<empty, error> {
#ifdef NOVA_LINUX
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(cfg.cpu, &cpu_set);

    const auto result_affinity = sched_setaffinity(cfg.pid, sizeof(cpu_set), &cpu_set);

    if (result_affinity == -1) {
        return unexpected<error>{ "Cannot set CPU affinity!" };
    }

    const auto result_priority = setpriority(
        PRIO_PROCESS,
        static_cast<unsigned int>(cfg.pid),
        std::to_underlying(cfg.priority)
    );

    if (result_priority == -1) {
        return unexpected<error>{ "Cannot set process priority!" };
    }
#else
// TODO: emit a non-error warning that every major compiler likes
// TODO(x-platform): emit a non-error warning that every major compiler likes
#endif  // NOVA_LINUX
    return empty{};
}


} // namespace nova
