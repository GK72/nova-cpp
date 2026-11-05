/**
 * Part of Nova C++ Library
 *
 * @brief   Converting the children of a node, stopping at the first error.
 *
 * @author      Gábor Krisztián Girhiny
 * @created     2026-10-31
 * @updated     2026-10-31
 * @status      Prototype
 */

#pragma once

#include <libnova/error.hpp>
#include <libnova/expected.hpp>

#include <concepts>
#include <cstddef>
#include <functional>
#include <utility>

namespace nova::detail {

/**
 * @brief   `convert` of each of `items`, in order, into a `Container` or the
 *          first error, if any.
 *
 * `convert` returns an `expected` of what goes into `Container`: an element,
 * or a key-value pair for a map, where the last of repeated keys wins. It
 * takes the item, and its index too if it can.
 */
template <typename Container, typename Items, typename Convert>
[[nodiscard]] auto collect(Items&& items, Convert&& convert) -> expected<Container, error> {
    auto ret = Container{ };
    std::size_t index = 0;

    for (auto&& item : items) {
        auto value = [&]() {
            if constexpr (std::invocable<Convert&, decltype(item), std::size_t>) {
                return std::invoke(convert, item, index++);
            } else {
                return std::invoke(convert, item);
            }
        }();

        if (not value) {
            return unexpected<error>{ std::move(value).error() };
        }

        if constexpr (requires { ret.insert_or_assign(std::move(value->first), std::move(value->second)); }) {
            ret.insert_or_assign(std::move(value->first), std::move(value->second));
        } else {
            ret.insert(ret.end(), std::move(*value));
        }
    }

    return ret;
}

} // namespace nova::detail
