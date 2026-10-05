#pragma once

#include "types/glaiel.hpp"
#include "types/rng.hpp"

#include <memory>

// Temporary CatData objects and the game RNG, ported from Amoeba (ffi/cat_factory.cpp).
//
// Exporter: cat_factory.cpp

struct CatDeleter {
    void operator()(CatData *c) const;
};
using TempCat = std::unique_ptr<CatData, CatDeleter>;

TempCat new_default_cat();
// A random stray of the game: unk_init + generated body parts (does not register anything).
TempCat make_random_stray();
// Fill `bp` with random game-generated body parts.
void generate_bodyparts(BodyParts *bp);

// The game's thread-local RNG (xoshiro256+). Only valid on the game thread.
Xoshiro256pContext &game_rng();

// breed registers the kitten's name in the name history through unk_init: suppress that while simulating.
extern bool g_suppress_name_history;
