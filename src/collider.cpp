#include "collider.h"
#include "rigel.h"

namespace rigel {

f32
do_sat_check_on_axis(AABB* aabb, CollisionTri* tri, glm::vec3 axis)
{
    auto aabb_proj = aabb->project_to(axis);
    auto tri_proj = tri->project_to(axis);

    if (aabb_proj.y <= tri_proj.x || tri_proj.y <= aabb_proj.x) {
        return -1.0;
    }
    auto depth = aabb_proj.y - tri_proj.x;
    auto other_depth = tri_proj.y - aabb_proj.x;
    depth = (other_depth < depth) ? other_depth : depth;

    return depth;
}

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
collide_AABB_with_static_AABB2(AABB* aabb, AABB* aabb_static, glm::vec3 displacement)
{
    CollisionResult result;
    result.t_to_collide = -1;
    AABB mink_diff;
    mink_diff.center = aabb->center - aabb_static->center;
    mink_diff.extents = aabb->extents + aabb_static->extents;

    if (abs(mink_diff.center.x) <= mink_diff.extents.x
        && abs(mink_diff.center.y) <= mink_diff.extents.y) {
        // we collide now
        glm::vec3 pen_vec = AABB_closest_to(&mink_diff, glm::vec3(0, 0, 0));
        auto mag = glm::length(pen_vec);
        result.penetration_axis = -pen_vec / mag;
        result.depth = mag;
        result.t_to_collide = 0;
        return result;
    }

    auto mink_displacement = -displacement;
    auto max_p = mink_diff.center + mink_diff.extents;
    auto min_p = mink_diff.center - mink_diff.extents;
    if (max_p.x > mink_displacement.x && max_p.y > mink_displacement.y
        && min_p.x < mink_displacement.x && min_p.y < mink_displacement.y) {
        // we will collide
        glm::vec3 pen_vec = AABB_closest_to(&mink_diff, mink_displacement);
        auto mag = glm::length(pen_vec);
        auto disp_mag = glm::length(displacement);
        result.penetration_axis = pen_vec / mag;
        result.depth = mag;
        result.t_to_collide = (disp_mag - mag) / disp_mag;
    }
    return result;
}

f32
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
    if (depth < 0) {
        auto aabb_future_min = aabb_min + displacement;
        auto aabb_future_max = aabb_max + displacement;
        f32 future_depth = do_ranges_overlap(aabb_future_min.x, aabb_future_max.x, aabb_static_min.x, aabb_static_max.x);
        if (future_depth < 0) {
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
    if (depth < 0) {
        auto aabb_future_min = aabb_min + displacement;
        auto aabb_future_max = aabb_max + displacement;
        f32 future_depth = do_ranges_overlap(aabb_future_min.y, aabb_future_max.y, aabb_static_min.y, aabb_static_max.y);
        if (future_depth < 0) {
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
        result.t_to_collide = 0;
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

CollisionResult
collide_AABB_with_static_AABB(AABB* aabb, AABB* aabb_static)
{
    CollisionResult result;
    f32 min_depth = INFINITY;

    auto left_edge = aabb->center.x - aabb->extents.x;
    auto right_edge = aabb->center.x + aabb->extents.x;
    auto left_edge_static = aabb_static->center.x - aabb_static->extents.x;
    auto right_edge_static = aabb_static->center.x + aabb_static->extents.x;
    if (right_edge <= left_edge_static || right_edge_static <= left_edge) {
        return result;
    }

    auto depth = right_edge - left_edge_static;
    auto other = right_edge_static - left_edge;
    depth = (other < depth) ? other : depth;
    if (depth < min_depth) {
        min_depth = depth;
        f32 component = (aabb->center.x > aabb_static->center.x) ? 1 : -1;
        result.penetration_axis = glm::vec3(component, 0, 0);
        result.vdist_to_out = 0;
    }

    auto top_edge = aabb->center.y + aabb->extents.y;
    auto bottom_edge = aabb->center.y - aabb->extents.y;
    auto top_edge_static = aabb_static->center.y + aabb_static->extents.y;
    auto bottom_edge_static = aabb_static->center.y - aabb_static->extents.y;
    if (top_edge <= bottom_edge_static || top_edge_static <= bottom_edge) {
        return result;
    }

    depth = top_edge - bottom_edge_static;
    other = top_edge_static - bottom_edge;
    depth = (other < depth) ? other : depth;
    if (depth < min_depth) {
        min_depth = depth;
        f32 component = (aabb->center.y > aabb_static->center.y) ? 1 : -1;
        result.penetration_axis = glm::vec3(0, component, 0);
        result.vdist_to_out = component * depth;
    }

    result.depth = min_depth;
    result.t_to_collide = 0;
    return result;
}

CollisionResult
collide_AABB_with_static_tri(AABB* aabb, glm::vec3 displacement, CollisionTri* tri)
{
    CollisionResult result;
    f32 min_depth = INFINITY;
    bool collision_in_future = false;
    result.t_to_collide = -1;
    f32 t_to_collide = -INFINITY;
    f32 min_vdist = INFINITY;

    AABB future_aabb;
    future_aabb.center = aabb->center + displacement;
    future_aabb.extents = aabb->extents;

    for (usize tri_axis = 0; tri_axis < 3; tri_axis++) {
        auto end = tri_axis + 1;
        if (end > 2) {
            end = 0;
        }
        auto edge = tri->vertices[end] - tri->vertices[tri_axis];
        auto norm = glm::normalize(glm::vec3(-edge.y, edge.x, 0));

        for (usize aabb_axis = 0; aabb_axis < 2; aabb_axis++) {
            glm::vec3 axis((1 - aabb_axis), aabb_axis, 0);
            f32 depth = do_sat_check_on_axis(aabb, tri, axis);

            if (depth < 0) {
                f32 future_depth = do_sat_check_on_axis(&future_aabb, tri, axis);
                if (future_depth < 0) {
                    return result;
                }
                collision_in_future = true;
                f32 axis_t = abs(glm::dot(displacement, axis));
                axis_t = (axis_t - future_depth) / axis_t;
                if (axis_t > result.t_to_collide) {
                    t_to_collide = axis_t;
                    min_depth = future_depth;
                    result.penetration_axis = axis;
                    if (axis.x == 0 && (-future_depth) < min_vdist) {
                        min_vdist = -future_depth;
                    }
                }
                result.edge[0] = tri->vertices[tri_axis];
                result.edge[1] = tri->vertices[end];
            }

            if (!collision_in_future && depth < min_depth) {
                min_depth = depth;

                result.penetration_axis = axis;
                result.edge[0] = tri->vertices[tri_axis];
                result.edge[1] = tri->vertices[end];

                if (axis.x == 0 && (-depth) < min_vdist) {
                    min_vdist = -depth;
                }
            }
        }

        f32 depth = do_sat_check_on_axis(aabb, tri, norm);
        if (depth < 0) {
            f32 future_depth = do_sat_check_on_axis(&future_aabb, tri, norm);
            if (future_depth < 0) {
                return result;
            }
            collision_in_future = true;
            f32 axis_t = abs(glm::dot(displacement, norm));
            axis_t = (axis_t - future_depth) / axis_t;
            if (axis_t > result.t_to_collide) {
                t_to_collide = axis_t;
                min_depth = future_depth;
                result.penetration_axis = norm;
                result.edge[0] = tri->vertices[tri_axis];
                result.edge[1] = tri->vertices[end];

                auto pen_vec = norm * future_depth;
                f32 vdist;
                if (edge.x == 0) {
                    vdist = pen_vec.y;
                } else {
                    vdist = (-(pen_vec.x * edge.y) / edge.x) + pen_vec.y;
                }
                if (vdist < min_vdist) {
                    min_vdist = vdist;
                }
            }
        }

        if (!collision_in_future && depth < min_depth) {
            min_depth = depth;

            result.penetration_axis = norm;
            result.edge[0] = tri->vertices[tri_axis];
            result.edge[1] = tri->vertices[end];

            auto pen_vec = norm * depth;
            f32 vdist;
            if (edge.x == 0) {
                vdist = pen_vec.y;
            } else {
                vdist = (-(pen_vec.x * edge.y) / edge.x) + pen_vec.y;
            }
            if (vdist < min_vdist) {
                min_vdist = vdist;
            }
        }

    }
    result.depth = min_depth;
    if (!collision_in_future) {
        result.t_to_collide = 0;
    } else {
        result.t_to_collide = t_to_collide;
        if (t_to_collide == 0) {
            std::cout << "uh oh" << std::endl;
        }
    }
    result.vdist_to_out = min_vdist;
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

ColliderRaycastResult
ray_intersect_tri(CollisionTri* tri, glm::vec3 ray_origin, glm::vec3 ray_dir)
{
    ColliderRaycastResult result;
    f32 min = INFINITY;
    for (int i = 0; i < 3; i++) {
        int other = i + 1;
        if (other >= 3) {
            other = 0;

        }

        glm::vec3 start = tri->vertices[i];
        glm::vec3 end = tri->vertices[other];

        glm::vec3 d_axis = end - start;
        glm::vec3 d_origin = start - ray_origin;
        f32 det = (d_axis.x * ray_dir.y) - (d_axis.y * ray_dir.x);
        if (det != 0) {
            // we could still intersect a perpendicular side
            f32 u = ((d_origin.y * d_axis.x) - (d_origin.x * d_axis.y)) / det;
            f32 v = (d_origin.y * ray_dir.x - d_origin.x * ray_dir.y) / det;
            // get the earliest time
            if (u < min && v >= 0 && v < 1) {
                min = u;
                result.edge[0] = start;
                result.edge[1] = end;
            }
        }

    }
    result.t = min;
    return result;
}

} // namespace rigel
