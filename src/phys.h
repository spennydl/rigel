#ifndef RIGEL_PHYS_H
#define RIGEL_PHYS_H

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

#include "collider.h"
#include "mem.h"
#include "rigel.h"
#include "world.h"

namespace rigel {
namespace phys {

void
resolve_stage_collisions(WorldChunk* world_chunk,
                         ColliderSet* stage_colliders,
                         mem::Arena* scratch_arena);

void
integrate_entity_positions(WorldChunk* world_chunk, float dt);
} // namespace phys
} // namespace rigel

#endif // RIGEL_PHYS_H
