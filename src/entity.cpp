#include "entity.h"
#include "resource.h"
#include "tilemap.h"
#include "collider.h"
#include "rigelmath.h"

#include <cstring>

namespace rigel {

void
entity_set_animation(Entity* entity, const char* anim_name)
{
    auto animations = get_anim_resource(entity->animations_id);
    auto animation = animations->animations.get(anim_name);

    entity->animation.start_frame = animation->start_frame;
    entity->animation.current_frame = animation->start_frame;
    entity->animation.end_frame = animation->end_frame;
    entity->animation.timer = 0.0f;
    entity->animation.threshold = animations->frames[0].duration_ms / 1000.0f;

    entity->current_animation = anim_name;
}
void
entity_update_animation(Entity* entity, const char* anim_name)
{
    if (strcmp(entity->current_animation, anim_name) != 0)
    {
        entity_set_animation(entity, anim_name);
    }
}

AABB
entity_get_collider(Entity* entity)
{
    AABB result = entity->colliders->aabbs[0];
    result.center = entity->position + result.extents;
    return result;
}

m::Vec3
get_dda_step(m::Vec3 displacement)
{
    m::Vec3 result = {0};

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
    m::Vec3 tl {aabb.center.x - aabb.extents.x,
                aabb.center.y + aabb.extents.y, 0.0f};
    m::Vec3 br {aabb.center.x + aabb.extents.x,
                aabb.center.y - aabb.extents.y, 0.0f};
    m::Vec2 tile_min = world_to_tiles(tl);
    m::Vec2 tile_max = world_to_tiles(br);

    for (isize tile_y = tile_min.y; tile_y <= tile_max.y; tile_y++)
    {
        for (isize tile_x = tile_min.x; tile_x <= tile_max.x; tile_x++)
        {
            if (tile_x < 0 ||
                tile_x >= WORLD_WIDTH_TILES ||
                tile_y < 0 ||
                tile_y >= WORLD_HEIGHT_TILES)
            {
                continue;
            }

            isize tile_index = tile_to_index(tile_x, tile_y);
            assert(tile_index > 0);
            if (tile_index >= WORLD_SIZE_TILES || tile_map->tiles[tile_index] == TileType::EMPTY)
            {
                continue;
            }
            AABB tile_aabb;
            tile_aabb.extents = {4.0f, 4.0f, 0.0f};
            tile_aabb.center = tiles_to_world(tile_x, tile_y) + tile_aabb.extents;

            m::Vec3 result = simple_AABB_overlap(aabb, tile_aabb);
            if (result.x > 0 || result.y > 0) {
                return true;
            }
        }
    }

    return false;
}


EntityMoveResult
move_entity(Entity* entity, TileMap* tile_map, f32 dt, f32 top_speed)
{
#if 1
    // semi-implicit euler
    m::Vec3 new_vel = entity->velocity + entity->acceleration * dt;
    if (m::abs(new_vel.x) > top_speed)
    {
        new_vel.x = top_speed * m::signof(new_vel.x);
    }

    if (m::abs(new_vel.x) < 0.0125)
    {
        new_vel.x = 0;
    }

    m::Vec3 new_pos = entity->position + new_vel * dt;
#else
    // velocity verlet
    f32 dt2 = dt*dt;
    m::Vec3 new_pos = entity->position + (entity->velocity * dt) + (entity->old_accel*dt2*0.5f);
    m::Vec3 new_acc = entity->acceleration;
    m::Vec3 new_vel = entity->velocity + ((entity->old_accel + new_acc) * dt * 0.5f);
    entity->old_accel = new_acc;
    if (m::abs(new_vel.x) > top_speed)
    {
        new_vel.x = top_speed * m::signof(new_vel.x);
    }

    if (m::abs(new_vel.x) < 0.0125)
    {
        new_vel.x = 0;
    }
#endif



    CollisionResult min_collision_result;

    m::Vec3 dest_pixel = m::floor(new_pos);
    m::Vec3 dest_fract = m::fract(new_pos);

    m::Vec3 entity_pixel_position = m::floor(entity->position);// + m::Vec3{0.5f, 0.5f, 0.0f};

    AABB entity_aabb = entity_get_collider(entity);
    b32 we_found_one = false;
    b32 collided[9] = {0};
    EntityMoveResult move_result{};
    move_result.collision_happened = false;
    f32 sqdist_to_dest;

    while (true) // TODO: we should really limit this
    {
        m::Vec3 current_displacement = new_pos - (entity_pixel_position);
        sqdist_to_dest = m::dot(current_displacement, current_displacement);
        m::Vec3 best_so_far = entity_pixel_position;
        we_found_one = false;

        for (i32 y = -1; y < 2; y++)
        {
            for (i32 x = -1; x < 2; x++)
            {
                m::Vec3 offset {(f32)x, (f32)y, 0.0f};
                m::Vec3 test_center = entity_pixel_position + offset;

                entity_aabb.center = test_center + entity_aabb.extents;

                m::Vec3 test_displacement = new_pos - test_center;
                f32 test_sqdist = m::dot(test_displacement, test_displacement);

                i32 i = ((2 - (y + 1)) * 3) + (x + 1);
                collided[i] = collides_with_level(entity_aabb, tile_map);

                bool contains_dest = test_center.x == dest_pixel.x && test_center.y == dest_pixel.y;

                if (contains_dest || test_sqdist <= sqdist_to_dest)
                {
                    if (!collided[i])
                    {
                        sqdist_to_dest = contains_dest ? 0 : test_sqdist;
                        best_so_far = test_center;
                        we_found_one = !(x == 0 && y == 0);
                    }
                    else
                    {
                        move_result.collision_happened = true;
                        // TODO(spencer): bouncing would go here
                    }
                }
                move_result.collided[i] = collided[i];
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
    new_pos = m::floor(new_pos);
    i32 x_check = new_vel.x > 0 ? 5 : 3;
    i32 y_check = new_vel.y > 0 ? 1 : 7;
    if (collided[x_check])
    {
        dest_fract.x = 0;
        new_vel.x = 0;
    }
    if (collided[y_check])
    {
        dest_fract.y = 0;
        new_vel.y = 0;
    }

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
        m::Vec3 to_dest = dest_pixel - new_pos;
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

    return move_result;
}

}
