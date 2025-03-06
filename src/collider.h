#ifndef RIGEL_COLLIDER_H
#define RIGEL_COLLIDER_H

#include "mem.h"
#include "rigel.h"
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

enum ColliderType {
    COLLIDER_NONE = 0,
    COLLIDER_AABB,
    COLLIDER_TRIANGLE
};

struct ColliderRef {
    ColliderType type;
    union {
        AABB* aabb;
        CollisionTri* tri;
    };
};

inline bool collider_ref_eq(ColliderRef& lhs, ColliderRef& rhs)
{
    if (lhs.type != rhs.type) {
        return false;
    }
    return (lhs.type == COLLIDER_AABB) ? lhs.aabb == rhs.aabb : lhs.tri == rhs.tri;
}

struct ColliderSet
{
    usize n_aabbs;
    usize n_tris;

    AABB* aabbs;
    CollisionTri* tris;
};

}

#endif // RIGEL_COLLIDER_H
