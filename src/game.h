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
    WorldChunk* active_world_chunk;
};

extern EntityPrototype entity_prototypes[EntityType_NumberOfTypes];

GameState*
load_game(mem::GameMem& memory);

inline GameState*
initialize_game_state(mem::GameMem& memory)
{
    GameState* state = memory.game_state_arena.alloc_simple<GameState>();
    return state;
}

void sim_one_tick(f32 timestep, GameState* game_state);
}


#endif // RIGEL_GAME_H
