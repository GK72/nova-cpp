#include <nova/nova.hh>

#include <cstdlib>

int main() {
    nova::log::init("Test");

    nova::log::info(
        "Nova version: v{}.{}.{} ({})",
        NovaVersionMajor,
        NovaVersionMinor,
        NovaVersionPatch,
        NovaGitHash
    );

    return EXIT_SUCCESS;
}
