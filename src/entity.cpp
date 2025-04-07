#include "entity.h"
#include "tilemap.h"
#include "collider.h"

#include <glm/glm.hpp>

namespace rigel {

f32 signof(f32 val)
{
    return (val < 0) ? -1.0 : (val == 0) ? 0 : 1;
}

f32 abs(f32 val)
{
    return val * signof(val);
}

void move_entity(Entity* entity, TileMap* tile_map, f32 dt)
{
    // we assume now that e->acceleration has been determined for the frame.
    f32 dt2 = dt*dt;

    // NOTE: changed to this simpler integration. It will introduce some error when acceleration
    // changes.
    // Old was velocity verlet:
    //
    // glm::vec3 new_pos = player->position + player->velocity*dt + player->acceleration*(dt2*0.5f);
    // glm::vec3 new_vel = player->velocity + (player->acceleration + new_acc)*(dt*0.5f);
    glm::vec3 new_pos = entity->position + entity->velocity*dt + entity->acceleration*(dt2*0.5f);
    glm::vec3 new_vel = entity->velocity + (entity->acceleration * dt);

    if (abs(new_vel.x) > 100) {
        new_vel.x = 100 * signof(new_vel.x);
    }

    AABB entity_aabb = entity->colliders->aabbs[0];
    entity_aabb.center = entity->position + entity_aabb.extents;

    glm::vec3 entity_displacement = new_pos - entity->position;
    AABB broad_entity_aabb;
    broad_entity_aabb.center = (entity_aabb.center) + (0.5f*entity_displacement);
    broad_entity_aabb.extents = entity_aabb.extents + glm::abs(0.5f*entity_displacement);

    glm::vec3 tl = glm::vec3(broad_entity_aabb.center.x - broad_entity_aabb.extents.x,
                                broad_entity_aabb.center.y + broad_entity_aabb.extents.y, 0);
    glm::vec3 br = glm::vec3(broad_entity_aabb.center.x + broad_entity_aabb.extents.x,
                                broad_entity_aabb.center.y - broad_entity_aabb.extents.y, 0);
    glm::vec2 tile_min = world_to_tiles(tl);
    glm::vec2 tile_max = world_to_tiles(br);

    for (isize tile_y = tile_min.y; tile_y <= tile_max.y; tile_y++) {
        for (isize tile_x = tile_min.x; tile_x <= tile_max.x; tile_x++) {
            entity_aabb.center = entity->position + entity_aabb.extents;

            usize tile_index = tile_to_index(tile_x, tile_y);
            if (tile_index >= WORLD_SIZE_TILES || tile_map->tiles[tile_index] == TileType::EMPTY) {
                continue;
            }

            AABB tile_aabb;
            tile_aabb.extents = glm::vec3(4, 4, 0);
            tile_aabb.center = tiles_to_world(tile_x, tile_y) + tile_aabb.extents;

            CollisionResult result = collide_AABB_with_static_AABB(&entity_aabb,
                                                                &tile_aabb,
                                                                entity_displacement);

            // TODO:
            // this is wrong.
            // we should loop all tiles to find the one with the _earliest_ t_to_collide
            // and resolve the collision with that, then repeat until the entity cannot
            // move anymore.
            if (result.t_to_collide >= 0) {
                assert(result.t_to_collide <= 1 && "more than one");
                if (result.t_to_collide == 0) { // embedded
                    // TODO: We need to disallow corrections that push the entity into
                    // a position where a tile is.
                    auto correction = result.penetration_axis * result.depth;
                    entity->position += correction;
                    new_pos += correction;
                    glm::vec3 norm = glm::vec3(-result.penetration_axis.y, result.penetration_axis.x, 0);
                    new_vel = glm::dot(new_vel, norm) * norm;
                } else {
                    entity->position += result.t_to_collide * entity_displacement;
                    glm::vec3 remainder = result.depth * result.penetration_axis;
                    glm::vec3 norm = glm::vec3(-result.penetration_axis.y, result.penetration_axis.x, 0);

                    new_pos = entity->position + (glm::dot(remainder, norm) * norm);
                    new_vel = glm::dot(new_vel, norm) * norm;
                    entity_displacement = new_pos - entity->position;
                }
                if (result.penetration_axis.y > 0) {
                    state_transition_air_to_land(entity);
                }
            }
        }
    }

    entity->position = new_pos;
    entity->velocity = new_vel;
}

}
