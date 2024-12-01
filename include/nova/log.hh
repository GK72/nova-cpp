/**
 * Part of Nova C++ Library.
 *
 * Logging utilies.
 */

#pragma once

#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/ansicolor_sink.h>
#include <spdlog/cfg/env.h>

#include <string>

namespace nova {

namespace detail {

inline auto init(spdlog::logger& logger) -> spdlog::logger& {
    logger.set_pattern("[%Y-%m-%d %H:%M:%S.%f %z] [%n @%t] %^[%l]%$ %v");
    return logger;
}

} // namespace detail


namespace log {

inline void load_env_levels() {
    spdlog::cfg::load_env_levels();
}

/**
 * @brief   Initialize logging.
 *
 * Format: [2024-03-16 21:22:25.542140 +01:00] [NAME @THREAD_ID] [info]
 */
inline auto init(const std::string& name) -> spdlog::logger& {
    spdlog::set_default_logger(spdlog::create<spdlog::sinks::ansicolor_stderr_sink_mt>(name));
    return detail::init(*spdlog::get(name));
}

template <typename ...Args>
inline void critical(spdlog::format_string_t<Args...> fmt, Args &&...args) {
    spdlog::critical(fmt, std::forward<Args>(args)...);
}

template <typename ...Args>
inline void error(spdlog::format_string_t<Args...> fmt, Args &&...args) {
    spdlog::error(fmt, std::forward<Args>(args)...);
}

template <typename ...Args>
inline void warn(spdlog::format_string_t<Args...> fmt, Args &&...args) {
    spdlog::warn(fmt, std::forward<Args>(args)...);
}

template <typename ...Args>
inline void info(spdlog::format_string_t<Args...> fmt, Args &&...args) {
    spdlog::info(fmt, std::forward<Args>(args)...);
}

template <typename ...Args>
inline void debug(spdlog::format_string_t<Args...> fmt, Args &&...args) {
    spdlog::debug(fmt, std::forward<Args>(args)...);
}

template <typename ...Args>
inline void trace(spdlog::format_string_t<Args...> fmt, Args &&...args) {
    spdlog::trace(fmt, std::forward<Args>(args)...);
}

} // namespace log

namespace topic_log {

/**
 * @brief   Initialize a topic logger.
 *
 * It is a no-op if the logger is already initialized.
 *
 * Format: [2024-03-16 21:22:25.542140 +01:00] [NAME @THREAD_ID] [info]
 *
 * ## Filtering example
 *
 * ```sh
 * SPDLOG_LEVEL=debug,<name>=error
 * ```
 */
inline void create(const std::string& name) {
    auto logger = spdlog::get(name);
    if (logger == nullptr) {
        logger = spdlog::create<spdlog::sinks::ansicolor_stderr_sink_mt>(name);
        detail::init(*logger);
    }
}

inline void create(const std::vector<std::string>& names) {
    for (const auto& name : names) {
        create(name);
    }
}

inline auto get(const std::string& name) -> spdlog::logger& {
    auto logger = spdlog::get(name);
    if (logger == nullptr) {
        logger = spdlog::default_logger();
    }
    return *logger;
}

// TODO: fallback to a default logger if not initialized

template <typename ...Args>
inline void critical(const std::string& name, spdlog::format_string_t<Args...> fmt, Args&&...args) {
    get(name).critical(fmt, std::forward<Args>(args)...);
}

template <typename ...Args>
inline void error(const std::string& name, spdlog::format_string_t<Args...> fmt, Args&&...args) {
    get(name).error(fmt, std::forward<Args>(args)...);
}

template <typename ...Args>
inline void warn(const std::string& name, spdlog::format_string_t<Args...> fmt, Args&&...args) {
    get(name).warn(fmt, std::forward<Args>(args)...);
}

template <typename ...Args>
inline void info(const std::string& name, spdlog::format_string_t<Args...> fmt, Args&&...args) {
    get(name).info(fmt, std::forward<Args>(args)...);
}

template <typename ...Args>
inline void debug(const std::string& name, spdlog::format_string_t<Args...> fmt, Args&&...args) {
    get(name).debug(fmt, std::forward<Args>(args)...);
}

template <typename ...Args>
inline void trace(const std::string& name, spdlog::format_string_t<Args...> fmt, Args&&...args) {
    get(name).trace(fmt, std::forward<Args>(args)...);
}

} // namespace topic_log

} // namespace nova
