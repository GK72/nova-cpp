#pragma once

#include <any>
#include <atomic>
#include <condition_variable>
#include <exception>
#include <mutex>

namespace nova {

template <typename T>
struct range {
    T low;
    T high;
};

struct context {
    std::any any {};

    std::atomic_bool keep_alive = true;
    std::exception_ptr exception = nullptr;
    std::mutex mtx {};
    std::condition_variable cv {};
};

} // namespace nova
