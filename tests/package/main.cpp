#include <libnova/nova.hpp>
#include <package-test/gen/build_info.hpp>

#include <cstdlib>

auto entrypoint([[maybe_unused]] auto args) -> int {
    nova::log::init("Test");
    nova::log::info("Nova version: {}", nova::library_full_version());
    nova::log::info("Package test version: {}", package_test::full_version());

    return EXIT_SUCCESS;
}

NOVA_MAIN(entrypoint);
