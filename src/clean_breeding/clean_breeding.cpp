#include "amoeboid.hpp"
#include "config.hpp"
#include "utilities/debug_console.hpp"

// Clean Breeding: mod logic. S1 skeleton, only logs that the DLL loaded.

void clean_breeding_init() {
    config_load();
    D::info("Clean Breeding loaded (version {}, exe hash OK, game {})", MOD_VERSION, EXE_VERSION);
}
