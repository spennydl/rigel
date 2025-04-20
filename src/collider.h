#ifndef RIGEL_COLLIDER_H
#define RIGEL_COLLIDER_H

#include "mem.h"
#include "rigel.h"
#include "tilemap.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace rigel {

struct Rectangle
{
    i32 x;
    i32 y;
    i32 w;
    i32 h;

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

struct ColliderSet
{
    usize n_aabbs;
    AABB* aabbs;
};

struct CollisionResult
{
    f32 t_to_collide;
    // depth along penetration axis
    f32 depth;
    // how much to move along y to get out
    f32 vdist_to_out;
    // gives the direction of the collision
    glm::vec3 penetration_axis;
    // the edge on which we have collided
    glm::vec3 edge[2];

    CollisionResult()
      : t_to_collide(-1.0)
      , depth(-1.0)
      , vdist_to_out(0.0)
      , penetration_axis(0, 0, 0)
      , edge{glm::vec3(0), glm::vec3(0)}
    {
    }
};

struct ColliderRaycastResult {
    // t value on ray where collision happens
    f32 t;
    // edge we hit
    glm::vec3 edge[2];

    ColliderRaycastResult()
    : t(0)
    , edge{glm::vec3(0), glm::vec3(0)}
    {}
};

inline f32
do_ranges_overlap(f32 start1, f32 end1, f32 start2, f32 end2)
{
    if (end1 < start2 || end2 < start1) {
        // we cannot overlap
        return -1;
    }
    if (start1 <= start2) {
        return end1 - start2;
    }
    return end2 - start1;
}

inline glm::vec3
simple_AABB_overlap(AABB left, AABB right)
{
    f32 depth_x = do_ranges_overlap(left.center.x - left.extents.x,
                                    left.center.x + left.extents.x,
                                    right.center.x - right.extents.x,
                                    right.center.x + right.extents.x);

    f32 depth_y = do_ranges_overlap(left.center.y - left.extents.y,
                                    left.center.y + left.extents.y,
                                    right.center.y - right.extents.y,
                                    right.center.y + right.extents.y);

    if (depth_x >= 0 && depth_y >= 0) {
        return (depth_x <= depth_y) ? glm::vec3(depth_x, 0, 0) : glm::vec3(0, depth_y, 0);
    }

    return glm::vec3(-1, -1, 0);
}

CollisionResult
collide_AABB_with_static_AABB(AABB* aabb, AABB* aabb_static, glm::vec3 displacement);
glm::vec3
AABB_closest_to(AABB* aabb, glm::vec3 test);
ColliderRaycastResult
ray_intersect_AABB(AABB* aabb, glm::vec3 ray_origin, glm::vec3 ray_dir);


}

#endif // RIGEL_COLLIDER_H
