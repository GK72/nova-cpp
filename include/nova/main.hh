/**
 * Part of Nova C++ Library.
 *
 * The modern main.
 */

#pragma once

#include "nova/error.hh"

#include <spdlog/spdlog.h>

#include <cstdlib>
#include <ranges>
#include <string_view>
#include <span>

#define NOVA_MAIN(func)                                                         \
    int main(int argc, char* argv[]) {                                          \
        try {                                                                   \
            const auto args = std::span(argv, static_cast<std::size_t>(argc));  \
            return func(args | std::views::transform(                           \
                [](const auto& x) { return std::string_view(x); })              \
            );                                                                  \
        } catch (nova::detail::exception_base& ex) {                            \
            spdlog::error(                                                      \
                "Exception caught in main: {}\n{}\n{}",                         \
                ex.what(),                                                      \
                ex.where(),                                                     \
                ex.backtrace()                                                  \
            );                                                                  \
        } catch (std::exception& ex) {                                          \
            spdlog::error("Exception caught in main: {}", ex.what());           \
        } catch (const char* msg) {                                             \
            spdlog::error("Exception caught in main: {}", msg);                 \
        } catch (...) {                                                         \
            spdlog::error("Unknown exception caught in main");                  \
        }                                                                       \
    }

#define NOVA_MAIN_ARG_PARSE(func, parse)                                        \
    int main(int argc, char* argv[]) {                                          \
        try {                                                                   \
            const auto args = parse(argc, argv);                                \
            if (not args.has_value()) {                                         \
                return EXIT_SUCCESS;                                            \
            }                                                                   \
            return func(*args);                                                 \
        } catch (nova::detail::exception_base& ex) {                            \
            spdlog::error(                                                      \
                "Exception caught in main: {}\n{}\n{}",                         \
                ex.what(),                                                      \
                ex.where(),                                                     \
                ex.backtrace()                                                  \
            );                                                                  \
        } catch (std::exception& ex) {                                          \
            spdlog::error("Exception caught in main: {}", ex.what());           \
        } catch (const char* msg) {                                             \
            spdlog::error("Exception caught in main: {}", msg);                 \
        } catch (...) {                                                         \
            spdlog::error("Unknown exception caught in main");                  \
        }                                                                       \
    }
