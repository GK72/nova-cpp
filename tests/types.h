#pragma once

#include <compare>

#include "string_utils.h"

struct Point {
    int x;      // NOLINT(misc-non-private-member-variables-in-classes)
    int y;      // NOLINT(misc-non-private-member-variables-in-classes)

    auto operator<=>(const Point&) const = default;
};

auto toString(Point p) {
    return utl::joinStr("\n",
        utl::joinStr(" ", "x =", p.x),
        utl::joinStr(" ", "y =", p.y)
    );
}
