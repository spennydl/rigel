#ifndef RIGEL_GAME_H
#define RIGEL_GAME_H

#include "rigel.h"
#include "entity.h"
#include "mem.h"
#include "render.h"

namespace rigel {

enum Direction
{
    Direction_Stay,
    Direction_Up,
    Direction_Right,
    Direction_Down,
    Direction_Left
};
inline m::Vec2
dir_to_vec(Direction d)
{
    switch (d)
    {
        case Direction_Up: return m::Vec2 { 0, 1 }; break;
        case Direction_Right: return m::Vec2 { 1, 0 }; break;
        case Direction_Down: return m::Vec2 { 0, -1 }; break;
        case Direction_Left: return m::Vec2 { -1, 0 }; break;
        case Direction_Stay: return m::Vec2 { 0, 0 }; break;
    }
    return m::Vec2 {0, 0};
}

struct WorldChunk;
// TODO: what is this and where does it go
struct GameState
{
    m::Vec2 overworld_dims;
    mem::SimpleList<WorldChunk*> overworld_grid;

    WorldChunk* first_world_chunk;
    WorldChunk* active_world_chunk;
};

extern EntityPrototype entity_prototypes[EntityType_NumberOfTypes];

GameState*
load_game(mem::GameMem& memory);

void
switch_world_chunk(mem::GameMem& mem, GameState* state, Direction dir);
Direction
check_for_level_change(Entity* player);
void
simulate_one_tick(mem::GameMem& memory, GameState* game_state, f32 dt, render::BatchBuffer* entity_batch_buffer);
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
