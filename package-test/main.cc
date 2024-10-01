#include <nova/nova.h>

#include <cstdlib>
#include <spdlog/spdlog.h>

int main() {
    nova::log_init("Test");
    spdlog::info("Nova version: v{}.{}.{}", NovaVersionMajor, NovaVersionMinor, NovaVersionPatch);

    return EXIT_SUCCESS;
}
