#ifndef RIGEL_COLLIDER_H
#define RIGEL_COLLIDER_H

#include "mem.h"
#include "rigel.h"
#include "rigelmath.h"

namespace rigel {

struct Rectangle
{
    f32 x;
    f32 y;
    f32 w;
    f32 h;

    m::Mat4 get_world_transform()
    {
        return m::translation_by(m::Vec3 { x, y, 0 }) *
               m::scale_by(m::Vec3 { w, h, 0.0f });
    }
};

struct AABB
{
    m::Vec3 center;
    m::Vec3 extents;
    bool is_colliding;
};

inline AABB
aabb_from_rect(Rectangle rect)
{
    return AABB{ .center  = m::Vec3 {rect.x, rect.y, 0.0f},
                 .extents = m::Vec3 {rect.w / 2, rect.h / 2, 0} };
}

inline Rectangle
rect_from_aabb(AABB aabb)
{
    auto min = aabb.center - aabb.extents;
    return Rectangle{ .x = min.x,
                      .y = min.y,
                      .w = (aabb.extents.x * 2.0f),
                      .h = (aabb.extents.y * 2.0f) };
}

// TODO: Do we need or want multiple colliders anymore?
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
    m::Vec3 penetration_axis;
    // the edge on which we have collided
    m::Vec3 edge[2];

    CollisionResult()
      : t_to_collide(-1.0)
      , depth(-1.0)
      , vdist_to_out(0.0)
      , penetration_axis{0, 0, 0}
      , edge{m::Vec3{0, 0, 0}, m::Vec3{0, 0, 0}}
    {
    }
};

struct ColliderRaycastResult {
    // t value on ray where collision happens
    f32 t;
    // edge we hit
    m::Vec3 edge[2];
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

inline m::Vec3
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
        return (depth_x <= depth_y) ? m::Vec3{depth_x, 0.0f, 0.0f} : m::Vec3{0.0f, depth_y, 0.0f};
    }

    return m::Vec3{-1.0f, -1.0f, 0.0f};
}

CollisionResult
collide_AABB_with_static_AABB(AABB* aabb, AABB* aabb_static, m::Vec3 displacement);
m::Vec3
AABB_closest_to(AABB* aabb, m::Vec3 test);
ColliderRaycastResult
ray_intersect_AABB(AABB* aabb, m::Vec3 ray_origin, m::Vec3 ray_dir);


}

#endif // RIGEL_COLLIDER_H
