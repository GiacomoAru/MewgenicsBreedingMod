#include "cat_factory.hpp"
#include "amoeboid.hpp"
#include "types/glaiel.hpp"
#include "utilities/function_hook.hpp"
#include "utilities/portal.hpp"

#include <cstring>

MAKE_STPORTAL(0, TLS0OFF_xoshiro256p_rng_context,
    Xoshiro256pContext, get_xoshiro256p_rng_context
)

MAKE_SFPORTAL(ADDRESS_glaiel__CatData_ctor,
    CatData *, __cdecl, glaiel__CatData_ctor,
    (CatData *thiss),
    (thiss)
)

MAKE_SFPORTAL(ADDRESS_glaiel__CatData_dtor,
    void, __cdecl, glaiel__CatData_dtor,
    (CatData *thiss),
    (thiss)
)

MAKE_SFPORTAL(ADDRESS_glaiel__CatData_unk_init,
    CatData *, __cdecl, glaiel__CatData_unk_init_call,
    (CatData *p_cat, void *ofstream_eliminated_by_opt, int32_t sex, bool register_in_name_history),
    (p_cat, ofstream_eliminated_by_opt, sex, register_in_name_history)
)

MAKE_SFPORTAL(ADDRESS_glaiel__CatData_unk_init_bodyparts,
    void, __cdecl, glaiel__CatData_unk_init_bodyparts_call,
    (BodyParts *p_bodyparts),
    (p_bodyparts)
)

bool g_suppress_name_history = false;
MAKE_SHOOK(0, ADDRESS_glaiel__CatData_unk_init,
    void, __cdecl, glaiel__CatData_unk_init,
    CatData *p_cat, void *ofstream_eliminated_by_opt, int32_t sex, bool register_in_name_history
) {
    glaiel__CatData_unk_init_hook.orig(p_cat, ofstream_eliminated_by_opt, sex,
        g_suppress_name_history ? false : register_in_name_history);
}

void CatDeleter::operator()(CatData *c) const {
    glaiel__CatData_dtor(c);
    operator delete(c);
}

TempCat new_default_cat() {
    CatData *c = static_cast<CatData *>(operator new(sizeof(CatData)));
    std::memset(static_cast<void *>(c), 0, sizeof(CatData));
    glaiel__CatData_ctor(c);
    return TempCat(c);
}

TempCat make_random_stray() {
    TempCat c = new_default_cat();
    glaiel__CatData_unk_init_call(c.get(), nullptr, 3, false);
    glaiel__CatData_unk_init_bodyparts_call(&c->body_parts);
    return c;
}

void generate_bodyparts(BodyParts *bp) {
    glaiel__CatData_unk_init_bodyparts_call(bp);
}

Xoshiro256pContext &game_rng() {
    return get_xoshiro256p_rng_context();
}
