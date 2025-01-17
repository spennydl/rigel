#include "phys.h"

namespace rigel {
namespace phys {

#if 0
bool aabb_mink_check(AABB mink_diff)
{
    return (std::abs(mink_diff.center.x) < mink_diff.extents.x)
        && (std::abs(mink_diff.center.y) < mink_diff.extents.y);
}

template <typename T> int sgn(T val) {
    return (T(0) < val) - (val < T(0));
}

float line_seg_ray_intersect(glm::vec3 start, glm::vec3 end, glm::vec3 ray)
{
    float x1 = start.x;
    float x2 = end.x;
    float x3 = 0;
    float x4 = ray.x;

    float y1 = start.y;
    float y2 = end.y;
    float y3 = 0;
    float y4 = ray.y;

    float num_t = ((x1 - x3) * (y3 - y4)) - ((y1 - y3) * (x3 - x4));
    float num_u = ((x1 - x2) * (y1 - y3)) - ((y1 - y2) * (x1 - x3));
    float denom = ((x1 - x2) * (y3 - y4)) - ((y1 - y2) * (x3 - x4));

    if (num_t == 0 && denom == 0) {
        // colinear
        return std::numeric_limits<float>::infinity();
    }

    if (denom == 0) {
        // parallel lines
        return std::numeric_limits<float>::infinity();
    }

    float num_sgn = sgn(num_u);
    float denom_sgn = sgn(num_u);
    // verify there is actually a collision before we do division
    if ((num_sgn == denom_sgn) && num_u < denom) {
        float t = num_t / denom;
        float u = -(num_u / denom);
        if ((t >= 0) && (t <= 1) && (u >= 0) && (u <= 1)) {
            return u;
        }
    }
    return std::numeric_limits<float>::infinity();
}

float aabb_minkowski(AABB mink_diff, glm::vec3 rel_vel)
{
    float min_t = std::numeric_limits<float>::infinity();

    for (auto line : mink_diff.to_lines()) {
        float intersect_t = line_seg_ray_intersect(line.first, line.second, rel_vel);
        if (intersect_t <= 1.0 && intersect_t >= 0.0 && intersect_t < min_t) {
            min_t = intersect_t;
        }
    }

    return min_t;
}
#endif
}
}
