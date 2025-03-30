#include "rigel.h"
#include "collider.h"

namespace rigel {

glm::vec3 AABB_closest_to(AABB* aabb, glm::vec3 test)
{
    auto my_min = aabb->center - aabb->extents;
    auto my_max = aabb->center + aabb->extents;
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
    if (result.x == 0 && result.y == 0) {
        result.x = -1;
    }
    return result;
}

CollisionResult
collide_AABB_with_static_AABB(AABB* aabb, AABB* aabb_static, glm::vec3 displacement)
{
    CollisionResult result;
    f32 min_depth = INFINITY;
    f32 t_to_collide = 0;
    bool collision_is_in_future = false;

    auto aabb_min = aabb->center - aabb->extents;
    auto aabb_max = aabb->center + aabb->extents;
    auto aabb_static_min = aabb_static->center - aabb_static->extents;
    auto aabb_static_max = aabb_static->center + aabb_static->extents;

    f32 depth = do_ranges_overlap(aabb_min.x, aabb_max.x, aabb_static_min.x, aabb_static_max.x);
    if (depth <= 0) {
        auto aabb_future_min = aabb_min + displacement;
        auto aabb_future_max = aabb_max + displacement;
        f32 future_depth = do_ranges_overlap(aabb_future_min.x, aabb_future_max.x, aabb_static_min.x, aabb_static_max.x);
        if (future_depth <= 0) {
            return result;
        }
        collision_is_in_future = true;
        min_depth = future_depth;
        f32 component = (aabb->center.x > aabb_static->center.x) ? 1 : -1;
        result.penetration_axis = glm::vec3(component, 0, 0);
        result.vdist_to_out = 0;
        f32 displacement_length = glm::length(displacement);
        f32 t = (displacement_length - future_depth) / displacement_length;
        if (t > t_to_collide) {
            t_to_collide = t;
        }
        if (component > 0) {
            // right edge
            result.edge[0].x = aabb_static_max.x;
            result.edge[0].y = aabb_static_max.y;
            result.edge[1].x = aabb_static_max.x;
            result.edge[1].y = aabb_static_min.y;
        } else {
            result.edge[0].x = aabb_static_min.x;
            result.edge[0].y = aabb_static_max.y;
            result.edge[1].x = aabb_static_min.x;
            result.edge[1].y = aabb_static_min.y;
        }
    } else {
        // collision is now
        min_depth = depth;
        f32 component = (aabb->center.x > aabb_static->center.x) ? 1 : -1;
        result.penetration_axis = glm::vec3(component, 0, 0);
        result.vdist_to_out = 0;
        if (component > 0) {
            // right edge
            result.edge[0].x = aabb_static_max.x;
            result.edge[0].y = aabb_static_max.y;
            result.edge[1].x = aabb_static_max.x;
            result.edge[1].y = aabb_static_min.y;
        } else {
            result.edge[0].x = aabb_static_min.x;
            result.edge[0].y = aabb_static_max.y;
            result.edge[1].x = aabb_static_min.x;
            result.edge[1].y = aabb_static_min.y;
        }
    }

    depth = do_ranges_overlap(aabb_min.y, aabb_max.y, aabb_static_min.y, aabb_static_max.y);
    if (depth <= 0) {
        auto aabb_future_min = aabb_min + displacement;
        auto aabb_future_max = aabb_max + displacement;
        f32 future_depth = do_ranges_overlap(aabb_future_min.y, aabb_future_max.y, aabb_static_min.y, aabb_static_max.y);
        if (future_depth <= 0) {
            return result;
        }

        if (!collision_is_in_future || future_depth < min_depth) {
            // set the depth
            collision_is_in_future = true;
            min_depth = future_depth;

            f32 displacement_length = glm::length(displacement);
            f32 component = (aabb->center.y > aabb_static->center.y) ? 1 : -1;

            result.penetration_axis = glm::vec3(0, component, 0);
            result.vdist_to_out = min_depth;

            f32 t = (displacement_length - future_depth) / displacement_length;
            if (t > t_to_collide) {
                t_to_collide = t;
            }
            result.vdist_to_out = future_depth;
            if (component > 0) {
                //top
                result.edge[0].x = aabb_static_min.x;
                result.edge[0].y = aabb_static_max.y;
                result.edge[1].x = aabb_static_max.x;
                result.edge[1].y = aabb_static_max.y;
            } else {
                result.edge[0].x = aabb_static_min.x;
                result.edge[0].y = aabb_static_min.y;
                result.edge[1].x = aabb_static_max.x;
                result.edge[1].y = aabb_static_min.y;
            }
        }
    }

    if (!collision_is_in_future && depth < min_depth) {
        min_depth = depth;
        f32 component = (aabb->center.y > aabb_static->center.y) ? 1 : -1;
        result.penetration_axis = glm::vec3(0, component, 0);
        result.vdist_to_out = min_depth;
        t_to_collide = 0;
        if (component > 0) {
            //top
            result.edge[0].x = aabb_static_min.x;
            result.edge[0].y = aabb_static_max.y;
            result.edge[1].x = aabb_static_max.x;
            result.edge[1].y = aabb_static_max.y;
        } else {
            result.edge[0].x = aabb_static_min.x;
            result.edge[0].y = aabb_static_min.y;
            result.edge[1].x = aabb_static_max.x;
            result.edge[1].y = aabb_static_min.y;
        }
    }

    result.t_to_collide = t_to_collide;
    result.depth = min_depth;
    return result;
}

ColliderRaycastResult
ray_intersect_AABB(AABB* aabb, glm::vec3 ray_origin, glm::vec3 ray_dir)
{
    glm::vec3 p0 = aabb->center - aabb->extents;
    glm::vec3 p1(aabb->center.x - aabb->extents.x, aabb->center.y  + aabb->extents.y, 0);
    f32 min_t = INFINITY;
    ColliderRaycastResult result;


    auto d_axis = p1 - p0;
    auto d_origin = p0 - ray_origin;
    auto det = (d_axis.x * ray_dir.y) - (d_axis.y * ray_dir.x);
    if (det != 0) {
        f32 u = ((d_origin.y * d_axis.x) - (d_origin.x * d_axis.y)) / det;
        f32 v = (d_origin.y * ray_dir.x - d_origin.x * ray_dir.y) / det;

        if (u < min_t && v >= 0 && v < 1) {
            min_t = u;
            result.edge[0] = p0;
            result.edge[1] = p1;
        }
    }

    glm::vec3 p2(aabb->center.x + aabb->extents.x, aabb->center.y + aabb->extents.y, 0);
    d_axis = p2 - p1;
    d_origin = p1 - ray_origin;
    det = (d_axis.x * ray_dir.y) - (d_axis.y * ray_dir.x);
    if (det != 0) {
        f32 u = ((d_origin.y * d_axis.x) - (d_origin.x * d_axis.y)) / det;
        f32 v = (d_origin.y * ray_dir.x - d_origin.x * ray_dir.y) / det;

        if (u < min_t && v >= 0 && v < 1) {
            min_t = u;
            result.edge[0] = p1;
            result.edge[1] = p2;
        }
    }

    glm::vec3 p3(aabb->center.x + aabb->extents.x, aabb->center.y - aabb->extents.y, 0);
    d_axis = p3 - p2;
    d_origin = p2 - ray_origin;
    det = (d_axis.x * ray_dir.y) - (d_axis.y * ray_dir.x);
    if (det != 0) {
        f32 u = ((d_origin.y * d_axis.x) - (d_origin.x * d_axis.y)) / det;
        f32 v = (d_origin.y * ray_dir.x - d_origin.x * ray_dir.y) / det;

        if (u < min_t && v >= 0 && v < 1) {
            min_t = u;
            result.edge[0] = p2;
            result.edge[1] = p3;
        }
    }

    d_axis = p0 - p3;
    d_origin = p3 - ray_origin;
    det = (d_axis.x * ray_dir.y) - (d_axis.y * ray_dir.x);
    if (det != 0) {
        f32 u = ((d_origin.y * d_axis.x) - (d_origin.x * d_axis.y)) / det;
        f32 v = (d_origin.y * ray_dir.x - d_origin.x * ray_dir.y) / det;

        if (u < min_t && v >= 0 && v < 1) {
            min_t = u;
            result.edge[0] = p3;
            result.edge[1] = p0;
        }
    }

    result.t = min_t;
    return result;
}

} // namespace rigel
