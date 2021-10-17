#include "nova.h"

using namespace nova::literals;

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

int main() {
    "example test - success"_test = []{
        return nova::eq(2, 2);
    };

    "example test - less than error message"_test = []{
        return nova::lt(2, 2);
    };

    "example test - compound type error message"_test = []{
        return nova::eq(Point{ 1, 3 }, Point{ 2, 4 });
    };
}
