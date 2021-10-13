#include "nova.h"

using namespace nova::literals;

int main() {
    "example test 1"_test = []{
        return nova::EQ(2, 2);
    };

    "example test 2 (failure)"_test = []{
        return nova::EQ(1, 2);
    };
}
