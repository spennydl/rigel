#include "collider.h"
#include "rigel.h"

namespace rigel {

f32
do_sat_check_on_axis(AABB* aabb, CollisionTri* tri, glm::vec3 axis)
{
    auto aabb_proj = aabb->project_to(axis);
    auto tri_proj = tri->project_to(axis);

    if (aabb_proj.y < tri_proj.x || tri_proj.y < aabb_proj.x) {
        return -1.0;
    }
    auto depth = aabb_proj.y - tri_proj.x;
    auto other_depth = tri_proj.y - aabb_proj.x;
    depth = (other_depth < depth) ? other_depth : depth;

    return depth;
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
    if (right_edge < left_edge_static || right_edge_static < left_edge) {
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
    if (top_edge < bottom_edge_static || top_edge_static < bottom_edge) {
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
    return result;
}

CollisionResult
collide_AABB_with_static_tri(AABB* aabb, CollisionTri* tri)
{
    CollisionResult result;
    f32 min_depth = INFINITY;

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
                return result;
            }

            if (depth < min_depth) {
                min_depth = depth;

                result.penetration_axis = axis;

                if (axis.x == 0) {
                    result.vdist_to_out = -depth;
                } //else {
                    //result.vdist_to_out = 0;
                //}
                // TODO: This case needs work! Do I calc something here?
                //auto pen_vec = axis * depth;
                //pen_vec = glm::dot(norm, pen_vec) * pen_vec;
                //result.vdist_to_out = (-(pen_vec.x * edge.y) / edge.x) + pen_vec.y;
            }
        }

        f32 depth = do_sat_check_on_axis(aabb, tri, norm);
        if (depth < 0) {
            return result;
        }

        if (depth < min_depth) {
            min_depth = depth;

            result.penetration_axis = norm;

            auto pen_vec = norm * depth;
            if (edge.x == 0) {
                result.vdist_to_out = pen_vec.y;
            } else {
                result.vdist_to_out = (-(pen_vec.x * edge.y) / edge.x) + pen_vec.y;
                std::cout << "Set " << result.vdist_to_out << std::endl;
            }
        }

    }
    result.depth = min_depth;
    return result;
}

} // namespace rigel
