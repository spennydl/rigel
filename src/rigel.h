#ifndef RIGEL_H_
#define RIGEL_H_

#include <cstdint>

namespace rigel
{

typedef uint8_t ubyte;
typedef int8_t ibyte;

typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef uint32_t usize;
typedef int32_t isize;
typedef uint64_t big_usize;
typedef int64_t big_isize;

typedef float f32;
// is this a hack?
#define F32_INF (1.0f / 0.0)
#define F32_NEG_INF (-1.0f / 0.0)
typedef double f64;

#define ONE_PAGE 4096
#define ONE_KB (1 << 10)
#define ONE_MB (1 << 20)

constexpr static f32 PLAYER_XACCEL = 200.0f;
constexpr static f32 GRAVITY       = -800.0f;
constexpr static f32 PLAYER_JUMP   = 200.0f;
constexpr static f32 MAX_XSPEED    = 200.0f;

typedef int32_t EntityId;
constexpr static EntityId ENTITY_ID_NONE = -1;
// TODO: is this a good idea?
constexpr static EntityId PLAYER_ENTITY_ID = 0;

constexpr static i64 UPDATE_TIME_NS = 16666667;//8333333;
constexpr static i64 RENDER_TIME_NS = 16666667;

// Globals
struct InputState {
    bool move_right_requested;
    bool move_left_requested;
    bool jump_requested;
};

extern InputState g_input_state;



}


#endif // RIGEL_H_
