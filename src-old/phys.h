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

// TODO: this should be in collider.h, no?
ColliderRef
get_closest_collider_along_ray(ColliderSet* stage_colliders, glm::vec3 origin, glm::vec3 direction, f32* out_t);

void
integrate_entity_positions(WorldChunk* world_chunk, float dt, mem::Arena* scratch_arena);
} // namespace phys
} // namespace rigel

#endif // RIGEL_PHYS_H
