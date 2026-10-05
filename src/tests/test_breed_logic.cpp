#include "clean_breeding/breed_logic.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>

int main() {
    // scaled_coi
    assert(scaled_coi(0.25, 0) == 0.25);
    assert(scaled_coi(0.25, 1) == 0.125);
    assert(scaled_coi(0.25, 2) == 0.0);
    assert(scaled_coi(0.25, 3) == 0.5);
    assert(scaled_coi(0.75, 3) == 1.0); // capped
    assert(scaled_coi(0.3, 9) == 0.3);  // unknown level = Vanilla

    const std::vector<std::string> parents = {"Pox", "None", "Flu"};
    
    // new disorder removed, inherited one kept
    DisorderSlot a[2] = {{"Pox", 1}, {"Schizophrenia", 1}};
    assert(remove_new_disorders(a, parents) == 1);
    assert(a[0].name == "Pox" && a[1].is_none() && a[1].level == 1);

    // inherited in slot 1, new in slot 0: new removed, slot 1 moves to slot 0
    DisorderSlot b[2] = {{"Schizophrenia", 1}, {"Flu", 2}};
    assert(remove_new_disorders(b, parents) == 1);
    assert(b[0].name == "Flu" && b[0].level == 2 && b[1].is_none());

    // both new: both removed; nothing to do on an empty kitten
    DisorderSlot d[2] = {{"A", 1}, {"B", 1}};
    assert(remove_new_disorders(d, parents) == 2 && d[0].is_none() && d[1].is_none());
    DisorderSlot e[2];
    assert(remove_new_disorders(e, parents) == 0 && e[0].is_none());

    // should_block
    assert(!should_block(0, 0.0) && !should_block(3, 0.0));
    assert(should_block(2, 0.99) && should_block(1, 0.49) && !should_block(1, 0.5));

    // second_chance_candidates
    const std::vector<std::string> pd = {"Pox", "EternalYouth"};
    assert((second_chance_candidates(pd, {"None", "None"}) == std::vector<std::string>{"Pox", "EternalYouth"}));
    assert(second_chance_candidates(pd, {"Pox", "None"}).empty());   // kitten already has one of the parent's
    assert(second_chance_candidates({"None", "None"}, {"None", "None"}).empty());

    std::puts("test_breed_logic OK");
    return 0;
}
