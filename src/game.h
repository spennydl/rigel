#ifndef RIGEL_GAME_H
#define RIGEL_GAME_H

#include "rigel.h"
#include "world.h"
#include "entity.h"
#include "mem.h"

namespace rigel {

// TODO: what is this and where does it go
struct GameState
{
    WorldChunk* first_world_chunk;
    WorldChunk* active_world_chunk;
};

extern EntityPrototype entity_prototypes[EntityType_NumberOfTypes];

enum ZoneTriggerActions
{
    ZoneTriggerActions_SwitchLevel = 0,
    ZoneTriggerActions_NumActions
};

GameState*
load_game(mem::GameMem& memory);

void
switch_world_chunk(mem::GameMem& mem, GameState* state, i32 index);

inline GameState*
initialize_game_state(mem::GameMem& memory)
{
    GameState* state = memory.game_state_arena.alloc_simple<GameState>();
    return state;
}

void sim_one_tick(f32 timestep, GameState* game_state);
}


#endif // RIGEL_GAME_H
