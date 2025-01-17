#ifndef RIGEL_PHYS_H
#define RIGEL_PHYS_H

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

#include "mem.h"
#include "rigel.h"
#include "world.h"

namespace rigel {
namespace phys {

struct Rectangle
{
    int x;
    int y;
    int w;
    int h;

    glm::mat4 get_world_transform()
    {
        glm::mat4 transform(1.0);
        transform = glm::translate(transform, glm::vec3(x, y, 0));
        transform = glm::scale(transform, glm::vec3(w, h, 0));
        return transform;
    }
};

#if 0
// TODO: major cleanup needed here.
// also wanna refactor all of collision detection.
struct AABB
{
    glm::vec3 center;
    glm::vec3 extents;

    glm::vec3 min() const { return center - extents; }

    glm::vec3 max() const { return center + extents; }

    AABB minkowski_diff(AABB other) const
    {
        // auto mink_bottom_left = min() - other.max();
        auto mink_extents = extents + other.extents;
        return AABB{ .center = center - other.center, .extents = mink_extents };
    }

    glm::vec3 closest_point_to(glm::vec3 test) const
    {
        auto my_min = min();
        auto my_max = max();
        glm::vec3 result(my_min.x, 0, 0);
        float min_dist = std::abs(my_min.x - test.x);

        if (std::abs(my_max.x - test.x) < min_dist) {
            min_dist = std::abs(test.x - my_max.x);
            result.x = my_max.x;
            result.y = 0;
        }
        if (std::abs(my_max.y - test.y) < min_dist) {
            min_dist = std::abs(my_max.y - test.y);
            result.x = 0;
            result.y = my_max.y;
        }
        if (std::abs(my_min.y - test.y) < min_dist) {
            min_dist = std::abs(my_min.y - test.y);
            result.x = 0;
            result.y = my_min.y;
        }
        return result;
    }

    // TODO: can we not have vector? does it matter? probly
    std::vector<std::pair<glm::vec3, glm::vec3>> to_lines() const
    {
        std::vector<std::pair<glm::vec3, glm::vec3>> result;
        auto min_p = min();
        auto max_p = max();
        result.push_back(
          { glm::vec3(min_p.x, min_p.y, 0), glm::vec3(min_p.x, max_p.y, 0) });
        result.push_back(
          { glm::vec3(min_p.x, max_p.y, 0), glm::vec3(max_p.x, max_p.y, 0) });
        result.push_back(
          { glm::vec3(max_p.x, max_p.y, 0), glm::vec3(max_p.x, min_p.y, 0) });
        result.push_back(
          { glm::vec3(max_p.x, min_p.y, 0), glm::vec3(min_p.x, min_p.y, 0) });
        return result;
    }
};

bool
aabb_mink_check(AABB mink_diff);
float
aabb_minkowski(AABB mink_diff, glm::vec3 rel_vel);

#endif

struct AABB
{
    glm::vec3 center;
    glm::vec3 extents;
    bool is_colliding;

    constexpr usize get_n_axes() { return 4; }

    void get_normals(glm::vec3* out)
    {
        // TODO: need to specialize for AABB!
        // AABB -> just basis vectors
        out[0].x = 1;
        out[0].y = 0;
        out[0].z = 0;

        out[1].x = 0;
        out[1].y = 1;
        out[1].z = 0;

        out[2].x = -1;
        out[2].y = 0;
        out[2].z = 0;

        out[3].x = 0;
        out[3].y = -1;
        out[3].z = 0;
    }

    glm::vec2 project_to(glm::vec3 axis)
    {
        // if (axis.y == 0) {
        // return axis.x * glm::vec2(center.x - extents.x, center.x +
        // extents.x);
        //} else if (axis.x == 0) {
        // return axis.y * glm::vec2(center.y - extents.y, center.y +
        // extents.y);
        //}

        f32 min = INFINITY;
        f32 max = -INFINITY;

        glm::vec3 p1 = center + extents;
        f32 proj1 = glm::dot(p1, axis);
        if (proj1 < min) {
            min = proj1;
        }
        if (proj1 > max) {
            max = proj1;
        }

        glm::vec3 p2(center.x + extents.x, center.y - extents.y, 0);
        f32 proj2 = glm::dot(p2, axis);
        if (proj2 < min) {
            min = proj2;
        }
        if (proj2 > max) {
            max = proj2;
        }

        glm::vec3 p3(center.x - extents.x, center.y + extents.y, 0);
        f32 proj3 = glm::dot(p3, axis);
        if (proj3 < min) {
            min = proj3;
        }
        if (proj3 > max) {
            max = proj3;
        }

        glm::vec3 p4 = center - extents;
        f32 proj4 = glm::dot(p4, axis);
        if (proj4 < min) {
            min = proj4;
        }
        if (proj4 > max) {
            max = proj4;
        }

        return glm::vec2(min, max);
    }
};

inline AABB
aabb_from_rect(Rectangle rect)
{
    return AABB{ .center = glm::vec3(rect.x, rect.y, 0),
                 .extents = glm::vec3(rect.w / 2, rect.h / 2, 0) };
}

inline Rectangle
rect_from_aabb(AABB aabb)
{
    auto min = aabb.center - aabb.extents;
    return Rectangle{ .x = (int)min.x,
                      .y = (int)min.y,
                      .w = (int)(aabb.extents.x * 2.0f),
                      .h = (int)(aabb.extents.y * 2.0f) };
}

struct CollisionTri
{
    glm::vec3 vertices[3];

    constexpr usize get_n_axes() { return 3; }
    void get_normals(glm::vec3* out)
    {
        auto first = glm::normalize(vertices[1] - vertices[0]);
        auto second = glm::normalize(vertices[2] - vertices[1]);
        auto third = glm::normalize(vertices[0] - vertices[2]);
        out[0] = glm::vec3(-first.y, first.x, 0);
        out[1] = glm::vec3(-second.y, second.x, 0);
        out[2] = glm::vec3(-third.y, third.x, 0);
    }

    glm::vec2 project_to(glm::vec3 axis)
    {
        glm::vec2 result(INFINITY, -INFINITY);
        for (int i = 0; i < 3; i++) {
            auto proj = glm::dot(vertices[i], axis);
            if (proj < result.x) {
                result.x = proj;
            }
            if (proj > result.y) {
                result.y = proj;
            }
        }
        return result;
    }
};

struct ColliderSet
{
    usize n_aabbs;
    usize n_tris;

    AABB* aabbs;
    CollisionTri* tris;
};

struct CollisionResult
{
    // will be -1.0 if no collision will occur
    f32 time_to_collision;
    // gives the direction of the collision
    glm::vec3 penetration_vector;

    CollisionResult()
      : time_to_collision(-1.0)
      , penetration_vector(0, 0, 0)
    {
    }
};

inline float
signof(float x)
{
    return (x >= 0) ? 1.0 : -1.0;
}

inline float
abs(float x)
{
    return x * signof(x);
}

// TODO: specialize for AABB and AABB
template<typename T, typename U>
CollisionResult
sat_collision_check(mem::Arena* scratch_arena,
                    T* first,
                    U* second,
                    glm::vec3 first_velocity,
                    glm::vec3 second_velocity)
{
    usize n_axes_first = first->get_n_axes();
    usize n_axes_second = second->get_n_axes();
    usize total_axes = n_axes_first + n_axes_second;

    // assume these are normalized!
    glm::vec3* normal_axes = scratch_arena->alloc_array<glm::vec3>(total_axes);
    first->get_normals(normal_axes);
    second->get_normals(normal_axes + n_axes_first);

    CollisionResult result;
    float min_depth = INFINITY;
    float time_to_collide = 0;
    bool collision_is_in_future = false;

    for (usize i = 0; i < total_axes; i++) {
        // should be a vector of min (x) & max (y) projections onto the normal
        // axis
        glm::vec2 first_proj = first->project_to(normal_axes[i]);
        glm::vec2 second_proj = second->project_to(normal_axes[i]);

        // We are doing too much work though, especially in the case of two
        // AABB's since the axes returned by them will be the same.

        if (first_proj.y < second_proj.x || second_proj.y < first_proj.x) {
            // we are not currently overlapping on this axis. Check to see if we
            // will collide.
            // std::cout << "norm: " << normal_axes[i].x << "," <<
            // normal_axes[i].y << std::endl; std::cout << first_proj.y << ","
            // << second_proj.x << std::endl; std::cout << second_proj.y << ","
            // << first_proj.x << std::endl;
            return result;
#if 0
            // TODO: I don't understand why sweeping doesn't work.
            float first_proj_vel = glm::dot(first_velocity, normal_axes[i]);
            auto first_proj_future = first_proj + first_proj_vel;
            float second_proj_vel = glm::dot(second_velocity, normal_axes[i]);
            auto second_proj_future = second_proj + second_proj_vel;

            // see if we cross during the movement

            auto first_diff = first_proj.y - second_proj.x;
            auto second_diff = second_proj.y - first_proj.x;
            auto first_future_diff = first_proj_future.y - second_proj_future.x;
            auto second_future_diff = second_proj_future.y - first_proj_future.x;
            //std::cout << "first future: " << first_diff << " second future: " << second_diff << std::endl;
            if (signof(first_diff) == signof(first_future_diff)
                && signof(second_diff) == signof(second_future_diff)) {
                std::cout << "first: " << first_diff << " " << first_future_diff << std::endl;
                std::cout << "second: " << second_diff << " " << second_future_diff << std::endl;
                // we do not cross on this axis during this movement.
                if (collision_is_in_future) {
                    //std::cout << "but we bailed" << std::endl;
                }
                return result;
            }
            // we will overlap at some time
            collision_is_in_future = true;
            std::cout << "found one " << std::endl;
            float diff;
            if (signof(first_diff) != signof(first_future_diff)) {
                diff = first_future_diff - first_diff;
            } else {
                diff = second_future_diff - second_diff;
            }
            //diff = diff / glm::length(relative_velocity);
            std::cout << diff << std::endl;

            if (diff > time_to_collide) {
                time_to_collide = diff;
                result.penetration_vector = normal_axes[i] * (1.0f - diff);
            }
#endif
        }

        // we are currently overlapping on this axis
        // this is irrelevant if the collision is not happening right now
        if (!collision_is_in_future) {
            float depth = first_proj.y - second_proj.x;
            float other_depth = second_proj.y - first_proj.x;
            depth = (other_depth < depth) ? other_depth : depth;

            if (depth < min_depth) {
                // std::cout << "depth " << depth << std::endl;
                result.penetration_vector = normal_axes[i] * depth;
                min_depth = depth;
            }
        }
    }
    // if we get here then we checked all axes and did not find one
    // that did not collide
    result.time_to_collision = time_to_collide;
    return result;
}

void
resolve_stage_collisions(WorldChunk* world_chunk,
                         ColliderSet* stage_colliders,
                         mem::Arena* scratch_arena)
{
    for (usize e = 0; e < world_chunk->next_free_entity_idx; e++) {
        auto entity = world_chunk->entities + e;
        auto entity_colliders = &entity->colliders;
        for (usize ec = 0; ec < entity_colliders->n_aabbs; ec++) {
            auto collider = entity_colliders->aabbs[ec];
            collider.center.x = entity->position.x + collider.extents.x;
            collider.center.y = entity->position.y + collider.extents.y;

            for (usize sc = 0; sc < stage_colliders->n_aabbs; sc++) {
                auto stage_collider = stage_colliders->aabbs + sc;
                // TODO: we do not need the entity velocities
                CollisionResult collision_result =
                  sat_collision_check(scratch_arena,
                                      &collider,
                                      stage_collider,
                                      entity->velocity,
                                      glm::vec3(0, 0, 0));

                if (collision_result.time_to_collision >= 0) {
                    entity->position += collision_result.penetration_vector;
                    glm::vec3 tangent_vec(
                      -collision_result.penetration_vector.y,
                      collision_result.penetration_vector.x,
                      0);
                    tangent_vec = glm::normalize(tangent_vec);
                    entity->velocity =
                      glm::dot(entity.velocity, tangent_vec) * tangent_vec;

                    // TODO: here is where we would set entity on ground state
                    stage_collider->is_colliding = true;
                } else {
                    stage_collider->is_colliding = false;
                }
            }
        }
    }
}

// first we run brains
// then we integrate positions
// then we run collision make adjustments as necessary

void
integrate_entity_positions(WorldChunk* world_chunk, float dt)
{
    auto stage_colliders = &world_chunk->active_stage->colliders;
    float dtdt = dt * dt;
    glm::vec3 gravity_vec(0, rigel::GRAVITY, 0);

    for (usize e = 0; e < world_chunk->next_free_entity_idx; e++) {
        auto entity = world_chunk->entities + e;
        glm::vec3 new_pos = entity->position + (entity->velocity * dt) +
                            (0.5 * entity.acceleration * dtdt);
        glm::vec3 new_accel = entity->forces + gravity_vec;
        glm::vec3 new_velocity =
          entity->velocity + (entity->acceleration + new_accel) * (0.5 * dt);

        entity->position = new_pos;
        entity->velocity = new_velocity;
        entity->acceleration = new_accel;
    }
}

} // namespace phys
} // namespace rigel

#endif // RIGEL_PHYS_H
