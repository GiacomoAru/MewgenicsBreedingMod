#pragma once

#include <algorithm>
#include <cstdint>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// Pure breeding rules of the mod (no game types), so tests/test_breed_logic.cpp can check them.

// coi handed to the game's breed, per inbreeding level: 0 Vanilla, 1 Mild, 2 None, 3 Hard (docs/DESIGN.md).
inline double scaled_coi(double coi, int level) {
    switch(level) {
        case 1: return coi * 0.5;
        case 2: return 0.0;
        case 3: return std::min(1.0, coi * 2.0);
        default: return coi;
    }
}

// The game's empty disorder slot is the string "None" with level 1 (docs/re_notes.md, S3).
struct DisorderSlot {
    std::string name = "None";
    int64_t level = 1;
    bool is_none() const { return name == "None" || name.empty(); }
};

// Inbreeding level 2: remove from the kitten every disorder that neither parent had and that is not in the whitelist
// (that is the roll of the inbreeding formula, including its 2% floor). Slots stay filled in order (game behaviour,
// re_notes S3): a remaining disorder in slot 1 moves to slot 0. Returns how many were removed.
inline int remove_new_disorders(DisorderSlot (&kitten)[2], const std::vector<std::string> &parent_disorders,
                                const std::set<std::string> &whitelist) {
    int removed = 0;
    for(auto &s : kitten) {
        if(s.is_none()) continue;
        bool from_parent = std::find(parent_disorders.begin(), parent_disorders.end(), s.name) != parent_disorders.end();
        if(!from_parent && !whitelist.contains(s.name)) {
            s = DisorderSlot{};
            removed++;
        }
    }
    if(kitten[0].is_none() && !kitten[1].is_none()) {
        std::swap(kitten[0], kitten[1]);
    }
    return removed;
}

// Heredity levels 1 (Mild) and 2 (None): is an inherited trait blocked? `r` is a uniform random number in [0, 1).
inline bool should_block(int heredity, double r) {
    return heredity == 2 || (heredity == 1 && r < 0.5);
}

// Heredity level 3 (Hard): "second chance" probabilities (docs/DESIGN.md).
inline constexpr double HARD_DISORDER_CHANCE = 0.15;
inline constexpr double HARD_DEFECT_CHANCE = 0.5;

// Hard mode, disorders: the disorders of one parent that may be copied to the kitten. Empty when the kitten already
// has one of that parent's disorders (the vanilla roll already passed it on). Whitelisted disorders and "None" are
// never candidates, so the whitelist is never amplified.
inline std::vector<std::string> second_chance_candidates(const std::vector<std::string> &parent_disorders,
                                                         const std::vector<std::string> &kitten_disorders,
                                                         const std::set<std::string> &whitelist) {
    std::vector<std::string> out;
    for(const auto &p : parent_disorders) {
        if(p == "None" || p.empty()) continue;
        if(std::find(kitten_disorders.begin(), kitten_disorders.end(), p) != kitten_disorders.end()) {
            return {};
        }
        if(!whitelist.contains(p)) {
            out.push_back(p);
        }
    }
    return out;
}
