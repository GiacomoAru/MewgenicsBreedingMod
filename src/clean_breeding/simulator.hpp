#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Breeding simulator: calls the game's breed (through our hook, so with the current settings) N times on
// temporary kittens and reports rates. Debug tool, driven from the menu.
//
// Exporter: simulator.cpp

struct SimRequest {
    int64_t parent_a = 0;
    int64_t parent_b = 0;
    double coi = 0;
    int n = 1000;
};

struct SimStatus {
    bool running = false;
    int done = 0;
    int total = 0;
    std::string report; // multi-line text, empty until a run finishes
};

// One line per cat in memory, for the menu's parent selectors. Refreshed on the game thread while the menu asks for it.
struct CatInfo {
    int64_t key;
    std::string label; // "Name (#key) M/F | disorders | N bad parts"
};
void simulator_want_cats();            // call every frame the cat list is on screen
std::vector<CatInfo> simulator_cats(); // last refreshed list, sorted by name

void simulator_start(const SimRequest &req);
// Run every fixed test case on synthetic parents (results in the status report and the log).
void simulator_run_suite(int n_per_case);

// Queue a run; it starts when no other run is active (used by [debug] test_simulation).
void simulator_enqueue(const SimRequest &req);
SimStatus simulator_status();

// Called once per game frame from the game thread (the RNG is thread-local).
void simulator_tick();
