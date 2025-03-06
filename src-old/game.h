#ifndef RIGEL_GAME_H
#define RIGEL_GAME_H

#include "entity.h"
#include "world.h"
#include "mem.h"

namespace rigel
{

// NOTE: if GameState ever gets a constructor then
// intialize_game_state will have to be updated.
struct GameState
{
    WorldChunk* active_world_chunk;
};


}

#endif // RIGEL_GAME_H
