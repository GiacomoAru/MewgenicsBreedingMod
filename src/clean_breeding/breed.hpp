#pragma once

#include <cstdint>

// Shared state of the breed hook.
//
// Exporter: breed.cpp

// True while the simulator calls breed: the hook then skips logging and snapshots.
extern bool g_breed_sim_active;

// The last real (non-simulated) breed call, for the menu's "Fill from last breeding".
struct LastBreed {
    bool valid = false;
    int64_t parent_a = 0;
    int64_t parent_b = 0;
    double coi = 0;
};
extern LastBreed g_last_breed;

// Load last_breed.txt (written by the hook) into g_last_breed, if present.
void breed_load_last();
