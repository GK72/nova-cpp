#include "nova.h"

#include <chrono>
#include <thread>
#include <vector>

using namespace std::literals::chrono_literals;
using namespace nova::literals;

int main() {
    "example test - timer (s)"_test = []{
        std::this_thread::sleep_for(2s);
        return nova::eq(1, 1);
    };

    "example test - timer (ms)"_test = []{
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
