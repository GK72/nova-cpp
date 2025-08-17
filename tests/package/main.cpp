#include <libnova/nova.hpp>

#include <cstdlib>

auto entrypoint([[maybe_unused]] auto args) -> int {
    nova::log::init("Test");
    nova::log::info("Nova version: {}", nova::library_version());

    return EXIT_SUCCESS;
}

NOVA_MAIN(entrypoint);
