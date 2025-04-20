#include "entity.h"
#include "tilemap.h"
#include "collider.h"
#include "rigelmath.h"

#include <cmath>
#include <glm/glm.hpp>

namespace rigel {

AABB
entity_get_collider(Entity* entity)
{
    AABB result = entity->colliders->aabbs[0];
    result.center = entity->position + result.extents;
    return result;
}

glm::vec3
get_dda_step(glm::vec3 displacement)
{
    glm::vec3 result(0);

    if (displacement.x == 0 && displacement.y == 0)
    {
        return result;
    }

    f32 step;
    if (m::abs(displacement.x) >= m::abs(displacement.y))
    {
        step = m::abs(displacement.x);
    }
    else
    {
        step = m::abs(displacement.y);
    }

    result.x = displacement.x / step;
    result.y = displacement.y / step;

    return result;
}

bool
collides_with_level(AABB aabb, TileMap* tile_map)
{
    glm::vec3 tl = glm::vec3(aabb.center.x - aabb.extents.x,
                             aabb.center.y + aabb.extents.y, 0);
    glm::vec3 br = glm::vec3(aabb.center.x + aabb.extents.x,
                             aabb.center.y - aabb.extents.y, 0);
    glm::vec2 tile_min = world_to_tiles(tl);
    glm::vec2 tile_max = world_to_tiles(br);

    for (isize tile_y = tile_min.y; tile_y <= tile_max.y; tile_y++)
    {
        for (isize tile_x = tile_min.x; tile_x <= tile_max.x; tile_x++)
        {
            usize tile_index = tile_to_index(tile_x, tile_y);
            if (tile_index >= WORLD_SIZE_TILES || tile_map->tiles[tile_index] == TileType::EMPTY)
            {
                continue;
            }
            AABB tile_aabb;
            tile_aabb.extents = glm::vec3(4, 4, 0);
            tile_aabb.center = tiles_to_world(tile_x, tile_y) + tile_aabb.extents;

            glm::vec3 result = simple_AABB_overlap(aabb, tile_aabb);
            if (result.x > 0 || result.y > 0) {
                return true;
            }
        }
    }

    return false;
}

void
move_entity(Entity* entity, TileMap* tile_map, f32 dt)
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
    //std::cout << "New pos is " << new_pos.x << "," << new_pos.y << std::endl;
    //std::cout << "v " << entity->velocity.x << "," << entity->velocity.y << std::endl;
    //std::cout << "a " << entity->acceleration.x << "," << entity->acceleration.y << std::endl;
    glm::vec3 new_vel = entity->velocity + (entity->acceleration * dt);

    bool is_requesting_move = g_input_state.move_left_requested || g_input_state.move_right_requested;
    if (!is_requesting_move)
    {
        // at slow update speeds we can sometimes overshoot 0
        if (m::signof(new_vel.x) != m::signof(entity->velocity.x)) {
            new_vel.x = 0;
        }
    }

    // TODO: should put all the constants in one place somewhere
    if (m::abs(new_vel.x) > 120)
    {
        new_vel.x = 120 * m::signof(new_vel.x);
    }

    if (m::abs(new_vel.x) < 0.0125)
    {
        new_vel.x = 0;
    }

    CollisionResult min_collision_result;


    glm::vec3 dest_pixel = new_pos;
    dest_pixel.x = floorf(dest_pixel.x);
    dest_pixel.y = floorf(dest_pixel.y);
    glm::vec3 dest_fract = glm::fract(new_pos);

    glm::vec3 entity_pixel_position = glm::floor(entity->position);

    AABB entity_aabb = entity_get_collider(entity);
    bool we_found_one = false;
    bool collided[9] = {0};
    f32 sqdist_to_dest;
    while (true) // TODO: we should really limit this
    {
        sqdist_to_dest = F32_INF;//glm::dot(displacement, displacement);
        glm::vec3 best_so_far = entity_pixel_position;
        we_found_one = false;

        for (i32 y = -1; y < 2; y++)
        {
            for (i32 x = -1; x < 2; x++)
            {
                glm::vec3 offset(x, y, 0);
                glm::vec3 test_pixel = entity_pixel_position + offset;
                entity_aabb.center = test_pixel + entity_aabb.extents;

                glm::vec3 test_displacement = new_pos - test_pixel;
                f32 test_sqdist = glm::dot(test_displacement, test_displacement);

                i32 i = ((2 - (y + 1)) * 3) + (x + 1);
                collided[i] = collides_with_level(entity_aabb, tile_map);
                if (test_sqdist <= sqdist_to_dest &&
                    !collided[i])
                {
                    sqdist_to_dest = test_sqdist;
                    best_so_far = test_pixel;
                    we_found_one = !(x == 0 && y == 0);
                }
            }
        }

        entity_pixel_position = best_so_far;
        if (!we_found_one)
        {
            new_pos = entity_pixel_position;
            break;
        }
    }

    // check if we can add the fractional part to velocity
    // NOTE: this is sufficient as long as the level colliders are all pixel-aligned.
    // If that ever changes then we will need a more sophisticated refinement step.

    entity_aabb.center = new_pos + entity_aabb.extents + dest_fract;
    if (sqdist_to_dest == 0 && !collides_with_level(entity_aabb, tile_map))
    {
        entity->position = new_pos + dest_fract;
    }
    else
    {
        entity->position = new_pos;
    }

    if (sqdist_to_dest != 0)
    {
        glm::vec3 to_dest = dest_pixel - new_pos;
        i32 x_idx = (to_dest.x > 0) ? 5 : 3;
        i32 y_idx = (to_dest.y > 0) ? 1 : 7;


        if (to_dest.x != 0 && collided[x_idx]) {
            //std::cout << "collided x" << std::endl;
            new_vel.x = 0;
        }
        if (to_dest.y != 0 && collided[y_idx]) {
            new_vel.y = 0;
        }
    }
   entity->velocity = new_vel;

}

}
