/**
 * Part of Nova C++ Library.
 *
 * Logging utilies.
 *
 * The following log levels are available both for the basic and topic logging.
 * - critical
 * - error
 * - warn
 * - info
 * - debug
 * - trace (for debugging state)
 * - devel (trace level logs only in debug builds: use `SPDLOG_LEVEL=trace`
 *   environment variable)
 *
 * `nova::log::init()` must be called after the sinks and logger are created.
 */

#pragma once

#include <fmt/format.h>
#include <memory>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>
#include <spdlog/cfg/env.h>
#include <spdlog/sinks/ansicolor_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/syslog_sink.h>

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
 *
 * By default, the logging level is configurable via environment variable.
 *
 * ```sh
 * SPDLOG_LEVEL=debug,<logger_name/topic>=off[,...]
 * ```
 */
inline auto init(const std::string& name = "default", bool env_config = true) -> spdlog::logger& {
    if (env_config) {
        load_env_levels();
    }

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

template <typename ...Args>
inline void devel(
        [[maybe_unused]] spdlog::format_string_t<Args...> fmt,
        [[maybe_unused]] Args&&...args)
{
    #ifndef NDEBUG
    spdlog::trace(fmt, std::forward<Args>(args)...);
    #endif
}

} // namespace log

namespace topic_log {

/**
 * @brief   Initialize a topic logger with `stderr` color logger.
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

/**
 * @brief   Initialize multiple topic loggers with `stderr` color logger.
 */
inline void create(const std::vector<std::string>& names) {
    for (const auto& name : names) {
        create(name);
    }
}

/**
 * @brief   Create a logger with custom sinks.
 *
 * No configuration happens, just registration.
 */
inline void create(const std::string& name, spdlog::sinks_init_list sinks) {
    auto logger = std::make_shared<spdlog::logger>(name, sinks);
    spdlog::register_logger(logger);
}

/**
 * @brief   Create multiple loggers with custom sinks.
 *
 * All loggers receive the same sinks.
 *
 * No configuration happens, just registrations.
 */
inline void create(const std::vector<std::string>& names, spdlog::sinks_init_list sinks) {
    for (const auto& name : names) {
        create(name, sinks);
    }
}

inline auto get(const std::string& name) -> spdlog::logger& {
    auto logger = spdlog::get(name);
    if (logger == nullptr) {
        logger = spdlog::default_logger();
    }
    return *logger;
}

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

template <typename ...Args>
inline void devel(
        [[maybe_unused]] const std::string& name,
        [[maybe_unused]] spdlog::format_string_t<Args...> fmt,
        [[maybe_unused]] Args&&...args)
{
    #ifndef NDEBUG
    get(name).trace(fmt, std::forward<Args>(args)...);
    #endif
}

} // namespace topic_log

} // namespace nova
