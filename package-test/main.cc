#include <nova/nova.hh>

#include <cstdlib>

auto entrypoint([[maybe_unused]] auto args) -> int {
    nova::log::init("Test");
    nova::log::info("Nova version: v{}.{}.{}", NovaVersionMajor, NovaVersionMinor, NovaVersionPatch);

    return EXIT_SUCCESS;
}

NOVA_MAIN(entrypoint);
