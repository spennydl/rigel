#include "phys.h"
#include <iostream>

namespace rigel {
namespace phys {

#if 0
void
handle_entity_collision(entity::Entity* entity, glm::vec3 surf_norm, f32 depth)
{
    glm::vec3 tangent_vec(surf_norm.y, -surf_norm.x, 0);

    // entity->velocity = glm::dot(entity->velocity, tangent_vec) * tangent_vec;

    // normal force
    glm::vec3 vforce(0, entity->forces.y, 0);
    glm::vec3 norm_force = -glm::dot(surf_norm, vforce) * surf_norm;
    std::cout << "norm " << norm_force.x << "," << norm_force.y << std::endl;

    // entity->forces.y += norm_force.y;
    f32 norm_face = signof(surf_norm.x);
    glm::vec3 friction((-norm_face) * surf_norm.y, (norm_face)*surf_norm.x, 0);

    f32 norm_mag = glm::length(norm_force);
    std::cout << "norm mag: " << norm_mag << std::endl;
    f32 coeff_of_friction = 0.1;
    auto fric_force = coeff_of_friction * norm_mag * friction;
    std::cout << "fric " << fric_force.x << "," << fric_force.y << std::endl;
    entity->forces += fric_force + norm_force;
    //
    // if we collide below then change to ground state
    if (surf_norm.y > 0) {
        entity->state_flags =
          entity->state_flags &
          (~(entity::STATE_JUMPING | entity::STATE_FALLING));
        entity->state_flags |= entity::STATE_ON_LAND;
    }
}
#endif

void
check_if_still_on_ground(entity::Entity* entity, ColliderSet* stage_colliders)
{
    auto entity_hspeed = abs(entity->velocity.x);
    // auto disp = (entity_hspeed > 1) ? entity_hspeed : 1;
    auto disp = 4;
    std::cout << disp << std::endl;
    auto collider = entity->colliders->aabbs[0];

    collider.center.x = entity->position.x + collider.extents.x;
    collider.center.y = entity->position.y + collider.extents.y - disp;

    f32 displacement = INFINITY;
    for (usize c = 0; c < stage_colliders->n_aabbs; c++) {
        auto aabb = stage_colliders->aabbs + c;
        CollisionResult aabb_result =
          collide_AABB_with_static_AABB(&collider, aabb);
        if (aabb_result.depth > 0) {
            // entity is on the ground, yep.
            // snap 'em to it
            std::cout << "axis of aabb: " << aabb_result.penetration_axis.x
                      << "," << aabb_result.penetration_axis.y << std::endl;
            f32 d = disp - aabb_result.vdist_to_out;
            if (d < displacement) {
                displacement = d;
            }
        }
    }

    for (usize c = 0; c < stage_colliders->n_tris; c++) {
        auto tri = stage_colliders->tris + c;
        CollisionResult tri_result =
          collide_AABB_with_static_tri(&collider, tri);
        if (tri_result.depth > 0) {
            // entity is on ground still
            std::cout << "axis of tri: " << tri_result.penetration_axis.x << ","
                      << tri_result.penetration_axis.y << std::endl;
            f32 d = disp - tri_result.vdist_to_out;
            if (d < displacement) {
                displacement = d;
            }
        }
    }
    if (displacement < INFINITY) {
        std::cout << "displacing by " << displacement << std::endl;
        entity->position.y -= displacement;
    } else {
        entity->state_flags = entity->state_flags & ~(entity::STATE_ON_LAND);
        entity->state_flags = entity->state_flags | entity::STATE_FALLING;
    }
}

void
resolve_stage_collisions(WorldChunk* world_chunk,
                         ColliderSet* stage_colliders,
                         mem::Arena* scratch_arena)
{
    for (usize e = 0; e < world_chunk->next_free_entity_idx; e++) {
        auto entity = world_chunk->entities + e;
        auto entity_colliders = entity->colliders;
        bool entity_has_collided = false;
        for (usize ec = 0; ec < entity_colliders->n_aabbs; ec++) {
            auto collider = entity_colliders->aabbs[ec];
            collider.center.x = entity->position.x + collider.extents.x;
            collider.center.y = entity->position.y + collider.extents.y;

            for (usize sc = 0; sc < stage_colliders->n_aabbs; sc++) {
                // TODO: scratch arena too small
                scratch_arena->reinit();
                auto stage_collider = stage_colliders->aabbs + sc;
                // TODO: we do not need the entity velocities
                CollisionResult collision_result =
                  collide_AABB_with_static_AABB(&collider, stage_collider);

                if (collision_result.depth >= 0) {
                    entity->position += collision_result.penetration_axis *
                                        collision_result.depth;

                    if ((entity->state_flags & entity::STATE_FALLING) &&
                        collision_result.penetration_axis.y > 0) {
                        std::cout << "yup did it here" << std::endl;
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

                    stage_collider->is_colliding = true;
                    entity_has_collided = true;
                } else {
                    stage_collider->is_colliding = false;
                }
            }

            collider.center.x = entity->position.x + collider.extents.x;
            collider.center.y = entity->position.y + collider.extents.y;

            for (int t = 0; t < stage_colliders->n_tris; t++) {
                scratch_arena->reinit();

                auto stage_collider = stage_colliders->tris + t;

                CollisionResult collision_result =
                  collide_AABB_with_static_tri(&collider, stage_collider);

                if (collision_result.depth >= 0) {
                    entity->position.y += collision_result.vdist_to_out;

                    if ((entity->state_flags & entity::STATE_FALLING) &&
                        collision_result.penetration_axis.y > 0) {
                        std::cout << "yup did it" << std::endl;
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

                    stage_collider->is_colliding = true;
                    entity_has_collided = true;
                } else {
                    stage_collider->is_colliding = false;
                }
            }
        }
        if ((entity->state_flags & entity::STATE_ON_LAND)) {
            check_if_still_on_ground(entity, stage_colliders);
        }
    }
}

// first we run brains
// then we integrate positions
// then we run collision make adjustments as necessary

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
        // std::cout << "vel " << entity->velocity.x << "," <<
        // entity->velocity.y
        //<< std::endl;
        entity->acceleration = new_accel;
    }
    resolve_stage_collisions(
      world_chunk, &world_chunk->active_stage->colliders, scratch_arena);
}

}
}
