#include "phys.h"
#include <iostream>

namespace rigel {
namespace phys {

// TODO: Should think about decomposing this a little more.
// I'm a little hesitant to make a PhysicsObject type
// but that may be the way to go?

void
resolve_stage_collisions(WorldChunk* world_chunk,
                         ColliderSet* stage_colliders,
                         mem::Arena* scratch_arena)
{
    for (usize e = 0; e < world_chunk->next_free_entity_idx; e++) {
        auto entity = world_chunk->entities + e;
        auto entity_colliders = entity->colliders;
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
                  sat_collision_check(scratch_arena,
                                      &collider,
                                      stage_collider,
                                      entity->velocity,
                                      glm::vec3(0, 0, 0));

                if (collision_result.time_to_collision >= 0) {
                    // TODO: This is aabb specific. Dunno why I hafta do it?
                    // Need to look into it.
                    if (stage_collider->center.x > collider.center.x) {
                        collision_result.penetration_vector.x *= -1;
                    }
                    if (stage_collider->center.y > collider.center.y) {
                        collision_result.penetration_vector.y *= -1;
                    }
                    entity->position += collision_result.penetration_vector;

                    glm::vec3 tangent_vec(
                      -collision_result.penetration_vector.y,
                      collision_result.penetration_vector.x,
                      0);
                    if (tangent_vec.x != 0 || tangent_vec.y != 0) {
                        tangent_vec = glm::normalize(tangent_vec);
                        entity->velocity =
                          glm::dot(entity->velocity, tangent_vec) * tangent_vec;
                    }

                    // TODO: here is where we would set entity on ground state
                    if (collision_result.penetration_vector.y > 0) {
                        entity->state_flags =
                          entity->state_flags &
                          (~(entity::STATE_JUMPING | entity::STATE_FALLING));
                        entity->state_flags |= entity::STATE_ON_LAND;
                    }
                    stage_collider->is_colliding = true;
                } else {
                    stage_collider->is_colliding = false;
                }
            }

            // TODO
            // for (int t = 0; t < stage_colliders->n_tris; t++) {
            // memory.scratch_arena.reinit();
            // auto stage_collider = &stage_colliders->tris[t];
            //
            // phys::CollisionResult collision_result =
            // phys::sat_collision_check(&memory.scratch_arena, &collider,
            // stage_collider,
            // entity.velocity * (float)dt,
            // glm::vec3(0, 0, 0));
            // if (collision_result.time_to_collision >= 0) {
            // level_adjustment += collision_result.penetration_vector;
            //
            // std::cout << "Tri collision wooooo" << std::endl;
            // std::cout << collision_result.time_to_collision << std::endl;
            // std::cout << collision_result.penetration_vector.x << ","
            //<< collision_result.penetration_vector.y << ","
            //<< std::endl;
            //}
            //
            //}
        }
    }
}

// first we run brains
// then we integrate positions
// then we run collision make adjustments as necessary

void
integrate_entity_positions(WorldChunk* world_chunk, f32 dt)
{
    f32 dtdt = dt * dt;
    glm::vec3 gravity_vec(0, rigel::GRAVITY, 0);

    for (usize e = 0; e < world_chunk->next_free_entity_idx; e++) {
        auto entity = world_chunk->entities + e;
        glm::vec3 new_pos = entity->position + (entity->velocity * dt) +
                            (0.5f * entity->acceleration * dtdt);
        glm::vec3 new_accel = entity->forces + gravity_vec;

        // TODO: friction should be dynamic, perhaps.
        // Maybe we only apply it when the player stops moving.
        //
        // This doesn't model a character running very well;
        // rather, it's modelling what it would be like if we
        // were dragging them across the floor. Hmmm.
        if ((entity->state_flags & entity::STATE_ON_LAND) &&
            entity->velocity.x != 0) {
            // friction
            f32 friction_dir = -signof(entity->velocity.x);
            // TODO: this should be perp to the normal which means
            // it's not gonna work on slopes right now
            f32 coeff_of_friction = 0.1;
            new_accel.x += friction_dir * coeff_of_friction * (-rigel::GRAVITY);
        }

        glm::vec3 new_velocity =
          entity->velocity + (entity->acceleration + new_accel) * (0.5f * dt);

        if (new_velocity.x > MAX_XSPEED) {
            new_velocity.x = MAX_XSPEED;
        }
        if (new_velocity.x < -MAX_XSPEED) {
            new_velocity.x = -MAX_XSPEED;
        }

        auto position_err = entity->position_err;
        position_err += glm::fract(new_pos);
        new_pos = glm::floor(new_pos);

        while (position_err.x >= 1.0f) {
            new_pos.x += 1;
            position_err.x -= 1;
        }
        while (position_err.y >= 1.0f) {
            new_pos.y += 1;
            position_err.y -= 1;
        }
        entity->position = new_pos;
        entity->position_err = position_err;

        entity->velocity = new_velocity;
        entity->acceleration = new_accel;
    }
}

}
}
