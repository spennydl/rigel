#include "phys.h"
#include "collider.h"
#include <iostream>

namespace rigel {
namespace phys {


ColliderRef
get_closest_collider_along_ray(ColliderSet* stage_colliders, glm::vec3 origin, glm::vec3 direction, f32* out_t)
{
    ColliderRef result;
    f32 min_t = INFINITY;
    for (usize i = 0; i < stage_colliders->n_aabbs; i++) {
       f32 t = ray_intersect_AABB(&stage_colliders->aabbs[i], origin, direction);
       //if (t >= 0 && t <= 1) {
           if (t >= 0 && t < min_t) {
               min_t = t;
               result.type = COLLIDER_AABB;
               result.aabb = &stage_colliders->aabbs[i];
           }
       //}
    }

    for (usize i = 0; i < stage_colliders->n_tris; i++) {
        f32 t = ray_intersect_tri(&stage_colliders->tris[i], origin, direction);
        if (t >= 0 && t < min_t) {
            min_t = t;
            result.type = COLLIDER_TRIANGLE;
            result.tri = &stage_colliders->tris[i];
        }
    }

    *out_t = min_t;
    return result;
}

void
place_entity_on_ground(entity::Entity* entity, ColliderSet* stage_colliders)
{
    auto collider = entity->colliders->aabbs[0];
    collider.center = entity->position + collider.extents;

    glm::vec3 left(collider.center.x - collider.extents.x, collider.center.y, 0);
    glm::vec3 right(collider.center.x + collider.extents.x, collider.center.y, 0);
    glm::vec3 center = collider.center;
    glm::vec3 down_dir(0, -1, 0);

    f32 displacement = INFINITY;
    f32 disp_left = INFINITY;
    f32 disp_right = INFINITY;
    f32 disp_center = INFINITY;

    auto closest_left = get_closest_collider_along_ray(stage_colliders, left, down_dir, &disp_left);
    auto closest_right = get_closest_collider_along_ray(stage_colliders, right, down_dir, &disp_right);
    auto closest_center = get_closest_collider_along_ray(stage_colliders, center, down_dir, &disp_center);
    //std::cout << disp_left << " -- " << disp_right << std::endl;

    displacement = disp_left;
    ColliderRef closest = closest_left;
    if (disp_right < displacement) {
        displacement = disp_right;
        closest = closest_right;
    }
    if (disp_center < displacement) {
        displacement = disp_center;
        closest = closest_center;
    }

    //std::cout << "try to snap down by " << displacement << std::endl;
    if (displacement < (2 * collider.extents.y)) {
        entity->position.y -= (displacement - collider.extents.y - 1); // snap to one pixel above the ground
    } else if ((displacement > 2 * collider.extents.y) || displacement == INFINITY){
        std::cout << "couldn't find becuase displacment was " << displacement << std::endl;
        entity->state_flags = entity->state_flags & ~(entity::STATE_ON_LAND);
        entity->state_flags = entity->state_flags | entity::STATE_FALLING;
    }
}

void
collide_entity_with_stage(entity::Entity* entity, ColliderSet* stage_colliders, glm::vec3 new_pos, f32 dt)
{
    glm::vec3 displacement = new_pos - entity->position;
    f32 dtdt = dt * dt;
    bool entity_has_collided_with_ground = false;
    auto entity_colliders = entity->colliders;
    for (usize ec = 0; ec < entity_colliders->n_aabbs; ec++) {
        auto collider = entity_colliders->aabbs[ec];
        collider.center = entity->position + collider.extents;

        for (usize sc = 0; sc < stage_colliders->n_aabbs; sc++) {
            auto stage_collider = stage_colliders->aabbs + sc;
            CollisionResult collision_result =
                collide_AABB_with_static_AABB2(&collider, stage_collider, displacement);

            if (collision_result.t_to_collide >= 0) {
                if (collision_result.t_to_collide > 0) {
                    std::cout << "AABB COLLIDE LATER " << collision_result.t_to_collide << std::endl;
                    auto correction = collision_result.t_to_collide * 0.1f;
                    entity->position += (collision_result.t_to_collide - correction) * displacement;
                    std::cout << collision_result.penetration_axis.x << "," << collision_result.penetration_axis.y << std::endl;
                } else {
                    std::cout << "AABB COLLIDE NOW " << collision_result.t_to_collide << std::endl;
                    entity->position += collision_result.penetration_axis *
                                        collision_result.depth;
                    std::cout << collision_result.penetration_axis.x << "," << collision_result.penetration_axis.y << std::endl;
                }

                if (collision_result.penetration_axis.y > 0) {
                    entity_has_collided_with_ground = true;
                }
                if ((entity->state_flags & entity::STATE_FALLING) &&
                    collision_result.penetration_axis.y > 0) {
                    entity->state_flags =
                        entity->state_flags & ~(entity::STATE_FALLING);
                    entity->state_flags =
                        entity->state_flags | entity::STATE_ON_LAND;
                    entity->velocity.y = 0;
                    entity->acceleration.y = 0;
                } else {
                    glm::vec3 tangent(-collision_result.penetration_axis.y,
                                        collision_result.penetration_axis.x,
                                        0);
                    entity->velocity =
                        glm::dot(entity->velocity, tangent) * tangent;
                }
                displacement = (entity->velocity * dt) +
                            (0.5f * entity->acceleration * dtdt);
                collider.center = entity->position + collider.extents;

                stage_collider->is_colliding = true;
            } else {
                stage_collider->is_colliding = false;
            }
        }

        for (int t = 0; t < stage_colliders->n_tris; t++) {

            auto stage_collider = stage_colliders->tris + t;

            CollisionResult collision_result =
                collide_AABB_with_static_tri(&collider, displacement, stage_collider);

            if (collision_result.t_to_collide >= 0) {
                // collide in the future!
                if (collision_result.t_to_collide > 0) {
                    std::cout << "TRI COLLIDE LATER" << std::endl;
                    std::cout << collision_result.penetration_axis.x << ","
                        << collision_result.penetration_axis.y << std::endl;
                    entity->position += collision_result.t_to_collide * displacement;
                } else {
                    std::cout << "TRI COLLIDE NOW" << std::endl;
                    entity->position.y += collision_result.vdist_to_out;
                }

                if (collision_result.penetration_axis.y > 0) {
                    entity_has_collided_with_ground = true;
                }
                if ((entity->state_flags & entity::STATE_FALLING) &&
                    collision_result.penetration_axis.y > 0) {
                    entity->state_flags =
                        entity->state_flags & ~(entity::STATE_FALLING);
                    entity->state_flags =
                        entity->state_flags | entity::STATE_ON_LAND;
                    entity->velocity.y = 0;
                    entity->acceleration.y = 0;
                } else {
                    glm::vec3 tangent(collision_result.penetration_axis.y,
                                        -collision_result.penetration_axis.x,
                                        0);
                    entity->velocity =
                        glm::dot(entity->velocity, tangent) * tangent;
                }

                displacement = (entity->velocity * dt) +
                            (0.5f * entity->acceleration * dtdt);
                collider.center = entity->position + collider.extents;


                stage_collider->is_colliding = true;
            } else {
                stage_collider->is_colliding = false;
            }
        }
    }
    if ((entity->state_flags & entity::STATE_ON_LAND) > 0) {
        place_entity_on_ground(entity, stage_colliders);
    }

}

void
integrate_entity_positions(WorldChunk* world_chunk,
                           f32 dt,
                           mem::Arena* scratch_arena)
{
    f32 dtdt = dt * dt;
    glm::vec3 gravity_vec(0, rigel::GRAVITY, 0);

    for (usize e = 0; e < world_chunk->next_free_entity_idx; e++) {
        auto entity = world_chunk->entities + e;

        glm::vec3 new_pos = entity->position + (entity->velocity * dt) +
                            (0.5f * entity->acceleration * dtdt);
        collide_entity_with_stage(entity, &world_chunk->active_stage->colliders, new_pos, dt);
        new_pos = entity->position + (entity->velocity * dt) +
                            (0.5f * entity->acceleration * dtdt);

        entity->position = new_pos;

        if (!(entity->state_flags & entity::STATE_ON_LAND)) {
            entity->forces += gravity_vec;
        }
        glm::vec3 new_accel = entity->forces;

        // TODO: we need to do collision here instead.
        //
        // Check the new position against the level. For each collision:
        // - zero out the normal component to the collision (as we do)
        // - get the normal force from the collision and use it to
        //   calculate friction

        glm::vec3 new_velocity =
          entity->velocity + (entity->acceleration + new_accel) * (0.5f * dt);

        if (new_velocity.x > MAX_XSPEED) {
            new_velocity.x = MAX_XSPEED;
        }
        if (new_velocity.x < -MAX_XSPEED) {
            new_velocity.x = -MAX_XSPEED;
        }
        if ((entity->state_flags & entity::STATE_JUMPING) &&
            new_velocity.y < 0) {
            entity->state_flags =
              entity->state_flags & ~(entity::STATE_JUMPING);
            entity->state_flags = entity->state_flags | entity::STATE_FALLING;
        }

        entity->velocity = new_velocity;
        entity->acceleration = new_accel;
    }
    //resolve_stage_collisions(
      //world_chunk, &world_chunk->active_stage->colliders, scratch_arena);
    //auto player = world_chunk->entities;
    //std::cout << player->position.x << "," << player->position.y << std::endl;
}

}
}
