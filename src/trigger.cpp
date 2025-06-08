#include "trigger.h"


namespace rigel {

#define X(NAME) \
    { .id = EffectId_##NAME, .fn = EFFECT_FN_NAME(NAME) },

EffectMap global_effects_map[EffectId_nEffects] = {
    EFFECTS_LIST
};
#undef X

}
