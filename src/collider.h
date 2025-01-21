#ifndef RIGEL_COLLIDER_H
#define RIGEL_COLLIDER_H

#include "mem.h"
#include "rigel.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace rigel {
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
    bool is_colliding;

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
    // depth along penetration axis
    f32 depth;
    // how much to move along y to get out
    f32 vdist_to_out;
    // gives the direction of the collision
    glm::vec3 penetration_axis;

    CollisionResult()
      : depth(-1.0)
      , vdist_to_out(0.0)
      , penetration_axis(0, 0, 0)
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

CollisionResult
collide_AABB_with_static_tri(AABB* aabb, CollisionTri* tri);
CollisionResult
collide_AABB_with_static_AABB(AABB* aabb, AABB* aabb_static);

#if 0
// TODO: specialize for AABB and AABB
template<typename T, typename U>
CollisionResult
sat_collision_check(mem::Arena* scratch_arena,
                    T* first,
                    U* second,
                    glm::vec3 first_displacement,
                    glm::vec3 second_displacement)
{
    usize n_axes_first = first->get_n_axes();
    usize n_axes_second = second->get_n_axes();
    usize total_axes = n_axes_first + n_axes_second;

    // assume these are normalized!
    glm::vec3* normal_axes = scratch_arena->alloc_array<glm::vec3>(total_axes);
    first->get_normals(normal_axes);
    second->get_normals(normal_axes + n_axes_first);

    CollisionResult result;
    f32 min_depth = INFINITY;
    f32 time_to_collide = -INFINITY;
    bool collision_is_in_future = false;

    for (usize i = 0; i < total_axes; i++) {
        // should be a vector of min (x) & max (y) projections onto the normal
        // axis
        glm::vec2 first_proj = first->project_to(normal_axes[i]);
        glm::vec2 second_proj = second->project_to(normal_axes[i]);

        // We are doing too much work though, especially in the case of two
        // AABB's since the axes returned by them will be the same.

        if (first_proj.y < second_proj.x || second_proj.y < first_proj.x) {
            return result;
#if 0
            // TODO: remove this continuous code if we don't need it
            //
            // for now we assume the second displacement is always 0
            auto first_disp_proj = glm::dot(first_displacement, normal_axes[i]);
            // auto second_disp_proj = glm::dot(second_displacement,
            // normal_axes[i]);
            auto first_future_proj = first_proj + first_disp_proj;
            // auto second_future_proj = second_proj + second_disp_proj;
            if (first_future_proj.y < second_proj.x ||
                second_proj.y < first_future_proj.x) {
                // we can fit this axis all the way thru the sweep
                return result;
            }
            collision_is_in_future = true;

            auto first_depth = first_future_proj.y - second_proj.x;
            auto other_depth = second_proj.y - first_future_proj.x;
            auto depth =
              (other_depth < first_depth) ? other_depth : first_depth;
            // TODO: this is wrong I think
            auto time =
              (abs(first_disp_proj) - abs(depth)) / abs(first_disp_proj);
            if (time > time_to_collide) {
                time_to_collide = time;
                result.penetration_axis = normal_axes[i];
            }
            continue;
#endif
        }

        if (!collision_is_in_future) {
            // we are currently overlapping on this axis
            auto depth = first_proj.y - second_proj.x;
            auto other_depth = second_proj.y - first_proj.x;
            depth = (other_depth < depth) ? other_depth : depth;

            if (depth < min_depth) {
                result.penetration_axis = normal_axes[i];

                min_depth = depth;
            }
        }
    }
    // if we get here then we checked all axes and did not find one
    // that did not collide
    result.time_to_collision = time_to_collide;
    result.depth = min_depth;
    return result;
}
#endif

}

#endif //
