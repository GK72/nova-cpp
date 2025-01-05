#include <nova/nova.hh>

#include <cstdlib>

int main() {
    nova::log::init("Test");
    spdlog::info("Nova version: v{}.{}.{}", NovaVersionMajor, NovaVersionMinor, NovaVersionPatch);

    return EXIT_SUCCESS;
}
