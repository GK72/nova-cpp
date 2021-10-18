#include "nova.h"

#include <chrono>
#include <thread>
#include <vector>

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

    "example test - timer (s)"_test = []{
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(2s);
        return nova::eq(1, 1);
    };

    "example test - timer (ms)"_test = []{
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(2ms);
        return nova::eq(1, 1);
    };

    "example test - timer (us)"_test = []{
        auto vec = std::vector<int>{};
        for (int i = 0; i < 100; ++i) {
            vec.push_back(1);
        }
        return nova::eq(1, 1);
    };

    "example test - timer (ns)"_test = []{
        return nova::eq(1, 1);
    };

}
