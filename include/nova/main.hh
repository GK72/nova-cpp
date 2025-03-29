/**
 * Part of Nova C++ Library.
 *
 * The modern main.
 */

#pragma once

#include "nova/log.hh"

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
        } catch (nova::exception& ex) {                                         \
            nova::log::error(                                                   \
                "Exception caught in main: {}\n{}\n",                           \
                ex.what(),                                                      \
                ex.where(),                                                     \
                ex.backtrace()                                                  \
            );                                                                  \
        } catch (std::exception& ex) {                                          \
            nova::log::error("Exception caught in main: {}", ex.what());        \
        } catch (const char* msg) {                                             \
            nova::log::error("Exception caught in main: {}", msg);              \
        } catch (...) {                                                         \
            nova::log::error("Unknown exception caught in main");               \
        }                                                                       \
        return EXIT_FAILURE;                                                    \
    }
