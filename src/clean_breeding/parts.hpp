#pragma once

#include "defect_table.hpp"
#include "types/glaiel.hpp"

#include <algorithm>
#include <cstdint>
#include <string_view>

// Body part helpers shared by the hook, the simulator and the snapshots.
//
// Slots 0-13 are the BodyPartDescriptors (arms and legs both use data/mutations/legs.gon), slot 14 is the texture.
// The game inherits left/right, arm1/arm2 and leg1/leg2 as pairs with the same id (docs/re_notes.md, S3): a "unit"
// is a set of slots that always change together.

inline constexpr int PART_COUNT = 14;
inline constexpr int SLOT_TEXTURE = 14;
inline constexpr int SLOT_COUNT = 15;

inline constexpr const char *PART_GROUPS[SLOT_COUNT] = {
    "body", "head", "tail", "legs", "legs", "legs", "legs", "eyes", "eyes", "eyebrows", "eyebrows", "ears", "ears", "mouth", "texture",
};

struct PartUnit {
    const char *group;
    int count;
    int slots[2];
};
inline constexpr PartUnit PART_UNITS[] = {
    {"body", 1, {0, 0}},   {"head", 1, {1, 1}},     {"tail", 1, {2, 2}},     {"legs", 2, {3, 4}},    {"legs", 2, {5, 6}},
    {"eyes", 2, {7, 8}},   {"eyebrows", 2, {9, 10}}, {"ears", 2, {11, 12}},  {"mouth", 1, {13, 13}}, {"texture", 1, {14, 14}},
};

inline const BodyPartDescriptor *part_of(const BodyParts &bp, int i) {
    const BodyPartDescriptor *slots[PART_COUNT] = {
        &bp.body, &bp.head, &bp.tail, &bp.leg1, &bp.leg2, &bp.arm1, &bp.arm2,
        &bp.lefteye, &bp.righteye, &bp.lefteyebrow, &bp.righteyebrow,
        &bp.leftear, &bp.rightear, &bp.mouth,
    };
    return slots[i];
}

inline BodyPartDescriptor *part_mut(BodyParts &bp, int i) {
    return const_cast<BodyPartDescriptor *>(part_of(bp, i));
}

// id of slot 0..14 (14 = texture)
inline int32_t part_id(const BodyParts &bp, int slot) {
    return slot == SLOT_TEXTURE ? static_cast<int32_t>(bp.texture_sprite_idx) : static_cast<int32_t>(part_of(bp, slot)->part_sprite_idx);
}

inline void set_part_id(BodyParts &bp, int slot, int32_t id) {
    if(slot == SLOT_TEXTURE) {
        bp.texture_sprite_idx = static_cast<uint32_t>(id);
    } else {
        part_mut(bp, slot)->part_sprite_idx = static_cast<uint32_t>(id);
    }
}

inline bool is_defect(std::string_view group, int32_t id) {
    for(const auto &g : DEFECT_GROUPS) {
        if(g.group == group) {
            return std::find(g.ids.begin(), g.ids.end(), id) != g.ids.end();
        }
    }
    return false;
}
