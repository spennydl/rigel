#ifndef RIGEL_TRIGGER_H
#define RIGEL_TRIGGER_H

#include "mem.h"
#include "collider.h"

namespace rigel {

struct GameState;
typedef void (*EffectFn)(mem::GameMem* memory, GameState* state, usize target_id, void* data);

#define EFFECT_FN_NAME(NAME) effect_##NAME
#define EFFECT_FN(NAME) void EFFECT_FN_NAME(NAME)(mem::GameMem* memory, GameState* game_state, usize target_id, void* data)

#define EFFECTS_LIST \
    X(ChangeLevel) \
    X(SpawnEntity)

enum EffectId
{
#define X(NAME) EffectId_##NAME,
EFFECTS_LIST
#undef X
    EffectId_nEffects
};

#define X(NAME) EFFECT_FN(NAME);
EFFECTS_LIST
#undef X

struct EffectMap
{
    EffectId id;
    EffectFn fn;
};

extern EffectMap global_effects_map[EffectId_nEffects];

#define TRIGGER_TYPES_LIST \
    X(ZoneTrigger) \
    X(InteractionTrigger)

enum TriggerType
{
#define X(NAME) TriggerType_##NAME,
    TRIGGER_TYPES_LIST
    TriggerType_NumTypes
#undef X
};

#define TEST_TRIGGER_FN(NAME) bool test_##NAME(GameState* game_state, NAME##Data* data)

#define X(NAME) \
    struct NAME##Data;                                          \
    TEST_TRIGGER_FN(NAME);
TRIGGER_TYPES_LIST
#undef X

#define TRIGGER_DATA_FIELDS \
    i32 id; \
    EffectId target_effect; \
    usize target_id; \
    void* effect_data;

struct ZoneTriggerData
{
    TRIGGER_DATA_FIELDS

    Rectangle rect;
};

struct InteractionTriggerData
{
    TRIGGER_DATA_FIELDS
};

}

#endif // RIGEL_TRIGGER_H
