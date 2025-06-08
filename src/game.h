#ifndef RIGEL_GAME_H
#define RIGEL_GAME_H

#include "rigel.h"
#include "entity.h"
#include "mem.h"

namespace rigel {

enum Direction
{
    Direction_Stay,
    Direction_Up,
    Direction_Right,
    Direction_Down,
    Direction_Left
};

struct WorldChunk;
// TODO: what is this and where does it go
struct GameState
{
    WorldChunk* first_world_chunk;
    WorldChunk* active_world_chunk;
};

extern EntityPrototype entity_prototypes[EntityType_NumberOfTypes];

GameState*
load_game(mem::GameMem& memory);

void
switch_world_chunk(mem::GameMem& mem, GameState* state, i32 index);
Direction
check_for_level_change(Entity* player);
void
simulate_one_tick(mem::GameMem& memory, GameState* game_state, f32 dt);
void
update_animations(WorldChunk* active_chunk, f32 dt);

inline GameState*
initialize_game_state(mem::GameMem& memory)
{
    GameState* state = memory.game_state_arena.alloc_simple<GameState>();
    return state;
}

}


#endif // RIGEL_GAME_H
