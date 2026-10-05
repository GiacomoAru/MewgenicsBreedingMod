#pragma once

#include "types/glaiel_ecs.hpp"
#include "types/msvc.hpp"

#include <cstddef>

// Minimal house types, from mewgenics-cat-bridge (types/glaiel_house.hpp), only the fields we read.

struct HouseRoom : Component {
    char _38[8];
    MsvcReleaseModeXString name; // "Floor1_Small", "Floor1_Large", "Attic", ...
};
static_assert(offsetof(HouseRoom, name) == 0x40);

struct HouseCat : Component {
    char _38[0xe8 - 0x38];
    // Room the cat lives in; nullptr for the daily stray waiting outside.
    HouseRoom *room;
};
static_assert(offsetof(HouseCat, room) == 0xe8);
