#include "debug.h"
#include "collider.h"
#include "rigel.h"
#include "mem.h"

namespace rigel {
namespace debug {

#ifdef RIGEL_DEBUG

struct Debug
{
    mem::Arena* debug_arena;
    mem::ArenaCheckpoint arena_checkpoint;

    usize next_free_line;
    DebugLine* lines;
};

static Debug* debug;

void do_alloc_debug()
{
    auto debug_arena = debug->debug_arena;
    debug->lines = debug_arena->alloc_array<DebugLine>(256);
    debug->next_free_line = 0;
}

void init_debug(mem::Arena* debug_arena)
{
    // TODO: is this what I want? this whole thing could probably
    // just be in static storage
    debug = debug_arena->alloc_simple<Debug>();
    debug->debug_arena = debug_arena;
    debug->arena_checkpoint = debug_arena->checkpoint();

    do_alloc_debug();
}

void new_frame()
{
    auto debug_arena = debug->debug_arena;
    debug_arena->restore_zeroed(debug->arena_checkpoint);

    do_alloc_debug();
}

void push_debug_line(DebugLine line)
{
    assert(debug->next_free_line < 128 && "Need more debug lines!");

    DebugLine* debug_line = debug->lines + debug->next_free_line;
    *debug_line = line;
    debug->next_free_line = debug->next_free_line + 1;
}

void push_rect_outline(Rectangle rect, m::Vec3 color)
{
    m::Vec3 bl = { rect.x, rect.y, 0.0f };
    m::Vec3 br = { rect.x + rect.w, rect.y, 0.0f };
    m::Vec3 tr = { rect.x + rect.w, rect.y + rect.h, 0.0f };
    m::Vec3 tl = { rect.x, rect.y + rect.h, 0.0f };

    push_debug_line({bl, color, br, color});
    push_debug_line({br, color, tr, color});
    push_debug_line({tr, color, tl, color});
    push_debug_line({tl, color, bl, color});
}

DebugLine* get_lines_for_frame(usize* out_count)
{
    *out_count = debug->next_free_line;
    return debug->lines;
}
#endif

} // namespace debug
} // namespace rigel
