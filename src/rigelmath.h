#ifndef RIGEL_MATH_H
#define RIGEL_MATH_H

#include <iostream>
#include <cassert>
// TODO: find an intrinsic for sqrt?
#include <cmath>

#include "rigel.h"


namespace rigel {
namespace m {


struct Vec2
{
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

    f32&
    operator[](usize i)
    {
        assert(i < 2 && "oob index");
        return xy[i];
    }

    const f32&
    operator[](usize i) const
    {
        assert(i < 2 && "oob index");
        return xy[i];
    }
};

struct Vec3
{
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

    f32&
    operator[](usize i)
    {
        assert(i < 3 && "oob index");
        return xyz[i];
    }

    const f32&
    operator[](usize i) const
    {
        assert(i < 3 && "oob index");
        return xyz[i];
    }
};

struct Vec4
{
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

    f32&
    operator[](usize i)
    {
        assert(i < 4 && "oob index");
        return xyzw[i];
    }

    const f32&
    operator[](usize i) const
    {
        assert(i < 4 && "oob index");
        return xyzw[i];
    }
};

inline Vec4
extend(Vec3 xyz, f32 w)
{
    return Vec4 { xyz.x, xyz.y, xyz.z, w };
}

inline std::ostream&
operator<<(std::ostream& os, Vec2& v)
{
    for (int i = 0; i < 2; i++)
    {
        os << v.xy[i] << "  ";
    }
    return os;
}

inline std::ostream&
operator<<(std::ostream& os, Vec3& v)
{
    for (int i = 0; i < 3; i++)
    {
        os << v.xyz[i] << "  ";
    }
    return os;
}

inline std::ostream&
operator<<(std::ostream& os, Vec4& v)
{
    for (int i = 0; i < 4; i++)
    {
        os << v.xyzw[i] << "  ";
    }
    return os;
}

struct Mat4
{
    Vec4 cols[4];

    inline Vec4&
    operator[](usize i)
    {
        assert(i < 4 && "oob");
        return cols[i];
    }

    inline const Vec4&
    operator[](usize i) const
    {
        assert(i < 4 && "oob");
        return cols[i];
    }
};

inline Mat4
mat4_I()
{
    return Mat4
    {{
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1},
    }};
}

inline Mat4
diag(f32 s)
{
    return Mat4
    {{
        {s, 0, 0, 0},
        {0, s, 0, 0},
        {0, 0, s, 0},
        {0, 0, 0, s},
    }};
}

inline Mat4
transpose(const Mat4& m)
{
    return Mat4
    {{
        {m[0].x, m[1].x, m[2].x, m[3].x},
        {m[0].y, m[1].y, m[2].y, m[3].y},
        {m[0].z, m[1].z, m[2].z, m[3].z},
        {m[0].w, m[1].w, m[2].w, m[3].w},
    }};
}

inline std::ostream&
operator<<(std::ostream& os, Mat4& mat)
{
    for (int m = 0; m < 4; m++)
    {
        for (int n = 0; n < 4; n++)
        {
            os << mat[n][m] << " ";
        }
        os << std::endl;
    }
    return os;
}

inline Vec4
operator*(const Mat4& left, const Vec4& right)
{
    Vec4 result {0, 0, 0, 0};
    for (int n = 0; n < 4; n++)
    {
        for (int k = 0; k < 4; k++)
        {
            result.xyzw[n] += left.cols[n][k] * right[n];
        }
    }
    return result;
}

inline Mat4
operator*(const Mat4& left, const Mat4& right)
{
    Mat4 result = {0};
    for (int n = 0; n < 4; n++)
    {
        for (int m = 0; m < 4; m++)
        {
            for (int k = 0; k < 4; k++)
            {
                result[m][n] += left[m][k] * right[k][n];
            }
        }
    }
    return result;
}

inline Mat4
operator*(const Mat4& left, const f32& right)
{
    Mat4 result = left;
    for (i32 n = 0; n < 4; n++)
    {
        for (i32 m = 0; m < 4; m++)
        {
            result[m][n] = left[m][n] * right;
        }
    }
    return result;
}

inline Mat4
translation_by(Vec3 by)
{
    Mat4 result = mat4_I();

    result.cols[3] = Vec4 { by.x, by.y, by.x, 1.0 };

    return result;
}

inline Mat4
scale_by(f32 by)
{
    Mat4 result = diag(by);

    result[3][3] = 1;

    return result;
}

inline Mat4
scale_by(Vec3 by)
{
    Mat4 result = mat4_I();

    result[0][0] = by.x;
    result[1][1] = by.y;
    result[2][2] = by.z;

    return result;
}

//--------------------------------------------------------------------------------
// Vec2 Operators
//--------------------------------------------------------------------------------

// Addition
inline Vec2
operator+(const Vec2& lhs, const Vec2& rhs)
{
    return Vec2 { lhs.x + rhs.x, lhs.y + rhs.y };
}

inline Vec2
operator+(const Vec2& lhs, const f32 rhs)
{
    return Vec2 { lhs.x + rhs, lhs.y + rhs };
}

inline Vec2
operator+(const f32 lhs, const Vec2 rhs)
{
    return rhs + lhs;
}

// Subtraction
inline Vec2
operator-(const Vec2& lhs, const Vec2& rhs)
{
    return Vec2 { lhs.x - rhs.x, lhs.y - rhs.y };
}

inline Vec2
operator-(const Vec2& lhs, const f32& rhs)
{
    return Vec2 { lhs.x - rhs, lhs.y - rhs };
}

inline Vec2
operator-(const f32& lhs, const Vec2& rhs)
{
    return Vec2 { lhs - rhs.x, lhs - rhs.y };
}

// Multiplication by a scalar. NO vector/vector mult,
// use a function to be clear what product you want
inline Vec2
operator*(const Vec2& lhs, const f32& rhs)
{
    return Vec2 { lhs.x * rhs, lhs.y * rhs };
}

inline Vec2
operator*(const f32& lhs, const Vec2& rhs)
{
    return rhs * lhs;
}

inline Vec2
operator/(const Vec2& lhs, const f32& rhs)
{
    return Vec2 { lhs.x / rhs, lhs.y / rhs};
}

inline bool
operator==(const Vec2& lhs, const Vec2& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

//--------------------------------------------------------------------------------
// Vec3 Operators
//--------------------------------------------------------------------------------

// Addition
inline Vec3
operator+(const Vec3& lhs, const Vec3& rhs)
{
    return Vec3 { lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z };
}

inline Vec3
operator+(const Vec3& lhs, const f32 rhs)
{
    return Vec3 { lhs.x + rhs, lhs.y + rhs, lhs.z + rhs };
}

inline Vec3
operator+(const f32 lhs, const Vec3 rhs)
{
    return rhs + lhs;
}

// Subtraction
inline Vec3
operator-(const Vec3& lhs, const Vec3& rhs)
{
    return Vec3 { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z };
}

inline Vec3
operator-(const Vec3& lhs, const f32& rhs)
{
    return Vec3 { lhs.x - rhs, lhs.y - rhs, lhs.z - rhs };
}

inline Vec3
operator-(const f32& lhs, const Vec3& rhs)
{
    return Vec3 { lhs - rhs.x, lhs - rhs.y, lhs - rhs.z };
}

// Multiplication by a scalar. NO vector/vector mult,
// use a function to be clear what product you want
inline Vec3
operator*(const Vec3& lhs, const f32& rhs)
{
    return Vec3 { lhs.x * rhs, lhs.y * rhs, lhs.z * rhs };
}

inline Vec3
operator*(const f32& lhs, const Vec3& rhs)
{
    return rhs * lhs;
}

inline Vec3
operator/(const Vec3& lhs, const f32& rhs)
{
    return Vec3 { lhs.x / rhs, lhs.y / rhs, lhs.z / rhs};
}

inline bool
operator==(const Vec3& lhs, const Vec3& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

//--------------------------------------------------------------------------------
// Vec4 Operators
//--------------------------------------------------------------------------------

// Addition
inline Vec4
operator+(const Vec4& lhs, const Vec4& rhs)
{
    return Vec4 { lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w };
}

inline Vec4
operator+(const Vec4& lhs, const f32 rhs)
{
    return Vec4 { lhs.x + rhs, lhs.y + rhs, lhs.z + rhs, lhs.w + rhs };
}

inline Vec4
operator+(const f32 lhs, const Vec4 rhs)
{
    return rhs + lhs;
}

// Subtraction
inline Vec4
operator-(const Vec4& lhs, const Vec4& rhs)
{
    return Vec4 { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w };
}

inline Vec4
operator-(const Vec4& lhs, const f32& rhs)
{
    return Vec4 { lhs.x - rhs, lhs.y - rhs, lhs.z - rhs, lhs.w - rhs };
}

inline Vec4
operator-(const f32& lhs, const Vec4& rhs)
{
    return Vec4 { lhs - rhs.x, lhs - rhs.y, lhs - rhs.z, lhs - rhs.w };
}

// Multiplication by a scalar. NO vector/vector mult,
// use a function to be clear what product you want
inline Vec4
operator*(const Vec4& lhs, const f32& rhs)
{
    return Vec4 { lhs.x * rhs, lhs.y * rhs, lhs.z * rhs, lhs.w * rhs };
}

inline Vec4
operator*(const f32& lhs, const Vec4& rhs)
{
    return rhs * lhs;
}

inline Vec4
operator/(const Vec4& lhs, const f32& rhs)
{
    return Vec4 { lhs.x / rhs, lhs.y / rhs, lhs.z / rhs, lhs.w / rhs};
}

inline f32
dot(const Vec2& lhs, const Vec2& rhs)
{
    return (lhs.x * rhs.x) + (lhs.y * rhs.y);
}

inline f32
dot(const Vec3& lhs, const Vec3& rhs)
{
    return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z + rhs.z);
}

inline f32
dot(const Vec4& lhs, const Vec4& rhs)
{
    return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z + rhs.z) + (lhs.w + rhs.w);
}

inline Vec2
hadamard(const Vec2& lhs, const Vec3& rhs)
{
    return Vec2 { lhs.x * rhs.x, lhs.y * rhs.y };
}

inline Vec3
hadamard(const Vec3& lhs, const Vec3& rhs)
{
    return Vec3 { lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z };
}

inline Vec4
hadamard(const Vec4& lhs, const Vec4& rhs)
{
    return Vec4 { lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w };
}

inline f32
length(Vec2 v)
{
    return sqrt(dot(v, v));
}

inline f32
length(Vec3 v)
{
    return sqrt(dot(v, v));
}

inline f32
length(Vec4 v)
{
    return sqrt(dot(v, v));
}


inline f32
signof_zero(f32 val)
{
    return (val < 0) ? -1.0 : (val == 0) ? 0 : 1;
}

inline f32
signof(f32 val)
{
    return (val < 0) ? -1.0f : 1.0f;
}

inline f32
abs(f32 val)
{
    return val * signof(val);
}

inline Vec2
abs(Vec2 val)
{
    return Vec2 {abs(val.x), abs(val.y)};
}

inline f32
floor(f32 val)
{
    return (f32)((i32)val);
}

inline Vec2
floor(Vec2 val)
{
    return {floor(val.x), floor(val.y)};
}

inline Vec3
floor(Vec3 val)
{
    return {floor(val.x), floor(val.y), floor(val.z)};
}
inline Vec4
floor(Vec4 val)
{
    return {floor(val.x), floor(val.y), floor(val.z), floor(val.w)};
}

inline f32
fract(f32 val)
{
    return val - floor(val);
}

inline Vec2
fract(Vec2 val)
{
    return {fract(val.x), fract(val.y)};
}

inline Vec3
fract(Vec3 val)
{
    return {fract(val.x), fract(val.y), fract(val.z)};
}
inline Vec4
fract(Vec4 val)
{
    return {fract(val.x), fract(val.y), fract(val.z), fract(val.w)};
}

inline f32
ceil(f32 val)
{
    return floor(val) + signof_zero(fract(val));
}

inline f32
round(f32 val)
{
    f32 fval = floor(val);
    return fval;
    f32 dist_to_floor = val - fval;
    return (dist_to_floor >= 0.5) ? fval + signof(val) : fval;
}

inline u64
dbj2(const char* str, usize len)
{
    u64 hash = 5381;
    for (usize i = 0; i < len && str[i]; i++)
    {
        hash = ((hash << 5) + hash) + str[i];
    }
    return hash;
}

}
}


#endif // RIGEL_MATH_H
