#include <nova/utils.h>

#include <cstdlib>
#include <spdlog/spdlog.h>

int main() {
    nova::log_init("Test");
    spdlog::info("Hello there");

    return EXIT_SUCCESS;
}
