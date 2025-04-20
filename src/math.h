#ifndef RIGEL_MATH_H
#define RIGEL_MATH_H

#include "rigel.h"

namespace rigel {
namespace m {


struct Vec2 {
    union
    {
        struct
        {
            f32 x;
            f32 y;
        };
        struct
        {
            f32 u;
            f32 v;
        };
        f32 xy[2];
        f32 uv[2];
    };
};

struct Vec3 {
    union
    {
        struct
        {
            f32 x;
            f32 y;
            f32 z;
        };
        struct
        {
            f32 u;
            f32 v;
            f32 w;
        };
        struct
        {
            f32 r;
            f32 g;
            f32 b;
        };
        f32 xyz[3];
        f32 uvw[3];
        f32 rgb[3];
    };
};

struct Vec4 {
    union
    {
        struct
        {
            f32 x;
            f32 y;
            f32 z;
            f32 w;
        };
        struct
        {
            f32 r;
            f32 g;
            f32 b;
            f32 a;
        };
        f32 xyzw[4];
        f32 rgba[4];
    };
};

Vec2
operator+(const Vec2& lhs, const Vec2& rhs)
{
    return Vec2 { lhs.x + rhs.x, lhs.y + rhs.y };
}

Vec2
operator-(const Vec2& lhs, const Vec2& rhs)
{
    return Vec2 { lhs.x - rhs.x, lhs.y - rhs.y };
}

inline f32
signof(f32 val)
{
    return (val < 0) ? -1.0 : (val == 0) ? 0 : 1;
}

inline
f32 abs(f32 val)
{
    return val * signof(val);
}

}
}


#endif // RIGEL_MATH_H
