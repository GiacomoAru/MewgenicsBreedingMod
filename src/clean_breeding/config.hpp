#pragma once

#include <set>
#include <string>
#include <utility>

// Settings, stored in config.ini next to the DLL (UTF-8 without BOM, ASCII values only).
// Level numbers: 0 = Vanilla, 1 = Mild, 2 = None, 3 = Hard (see docs/DESIGN.md).
//
// Exporter: config.cpp

enum class CleanseMode { Disorders, Defects, All };

struct Config {
    int inbreeding = 0;
    int heredity = 0;
    CleanseMode cleanse_mode = CleanseMode::All;
    std::set<std::string> whitelist_disorders;
    std::set<std::pair<std::string, int>> whitelist_defects; // (group, id), e.g. ("head", 704)
};

Config &config();

// Read config.ini into config(); missing or invalid values fall back to the defaults.
void config_load();

// Write the changed setting to config.ini immediately.
void config_save_breeding();
void config_save_cleanse_mode();
