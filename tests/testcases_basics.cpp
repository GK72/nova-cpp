#include "nova.h"

#include "types.h"

using namespace nova::literals;

int main() {
    "equality comparison test - success"_test = []{
        return nova::eq(2, 2);
    };

    "equality comparison test - error message"_test = []{
        return nova::eq(1, 2);
    };

    "less than comparison test - success"_test = []{
        return nova::lt(1, 2);
    };

    "less than comparison test - error message"_test = []{
        return nova::lt(2, 2);
    };

    "compound type comparison test - success"_test = []{
        return nova::eq(Point{ 2, 4 }, Point{ 2, 4 });
    };

    "compound type comparison test - error message"_test = []{
        return nova::eq(Point{ 1, 3 }, Point{ 2, 4 });
    };

}
