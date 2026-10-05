#pragma once

#include <set>
#include <string>
#include <utility>

// Settings, stored in config.ini next to the DLL (UTF-8 without BOM, ASCII values only).
// Level values in the file: 0 = Normal, 1 = Reduced, 2 = Off, 3 = Increased (see docs/DESIGN.md).
//
// Exporter: config.cpp

// Player-facing name of a level value (menu, logs, reports): 2 Off, 1 Reduced, 0 Normal, 3 Increased.
inline const char *level_label(int level) {
    switch(level) {
        case 1: return "Reduced";
        case 2: return "Off";
        case 3: return "Increased";
        default: return "Normal";
    }
}

struct Config {
    int inbreeding = 0;
    int heredity = 0;
#if CB_DEV_TOOLS
    bool force_default_style = false; // [debug] force_default_style=1: skip the game look to test the fallback
    bool developer_tools = false; // [debug] developer_tools=1 in config.ini; never settable from the menu
#endif
};

Config &config();

// Read config.ini into config(); missing or invalid values fall back to the defaults.
void config_load();

// Write the changed setting to config.ini immediately.
void config_save_breeding();
