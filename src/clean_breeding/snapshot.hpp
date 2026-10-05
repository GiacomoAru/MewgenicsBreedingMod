#pragma once

#include "types/glaiel.hpp"

#include <cstdint>
#include <vector>

// Dumps the state of every cat in memory to JSON files (next to the DLL, in snapshots\),
// after each call to breed. Debug/analysis tool, see docs/re_notes.md.
//
// Exporter: snapshot.cpp

// Called from the breed hook after the original returned. Copies what it needs.
void snapshot_note_breed(int call_no, const CatData &parent_a, const CatData &parent_b, double coi_param, const CatData &kitten);

// Cats currently in memory (house scene), by sql_key. nullptr if not found.
CatData *find_cat(int64_t sql_key);
size_t cat_count();
std::vector<CatData *> all_cats();