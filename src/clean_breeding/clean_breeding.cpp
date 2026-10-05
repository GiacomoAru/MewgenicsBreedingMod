#include "amoeboid.hpp"
#if CB_DEV_TOOLS
#include "breed.hpp"
#endif
#include "config.hpp"
#include "utilities/debug_console.hpp"

// Unnatural Selection: mod startup (settings, then a log line that the mod is active).

void clean_breeding_init() {
    config_load();
#if CB_DEV_TOOLS
    breed_load_last();
#endif
    D::info("{} loaded (version {}, exe hash OK, game {})", MOD_NAME, MOD_VERSION, EXE_VERSION);
}
