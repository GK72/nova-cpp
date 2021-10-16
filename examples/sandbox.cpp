#include "nova.h"

using namespace nova::literals;

struct Type {
    int value;
};

auto toString(Type x) {
    return utl::joinStr(" ", "Type's value:", x.value);
}

bool operator==(Type lhs, Type rhs) {
    return lhs.value == rhs.value;
}

int main() {
    "example test 1"_test = []{
        return nova::eq(2, 2);
    };

    "example test 2"_test = []{
        return nova::eq(1, 2);
    };

    "example test 3"_test = []{
        return nova::lt(1, 2);
    };

    "example test 4"_test = []{
        return nova::lt(2, 2);
    };

    "example test 5"_test = []{
        return nova::eq(Type{ 1 }, Type{ 2 });
    };
}
