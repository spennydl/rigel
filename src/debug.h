#ifndef RIGEL_DEBUG_H
#define RIGEL_DEBUG_H

#include "rigel.h"
#include "rigelmath.h"
#include "mem.h"
#include "collider.h"

namespace rigel {
namespace debug {

struct DebugLine
{
    m::Vec3 start;
    m::Vec3 start_color;
    m::Vec3 end;
    m::Vec3 end_color;
};

void init_debug(mem::Arena* debug_arena);
void new_frame();
void push_debug_line(DebugLine line);
void push_rect_outline(Rectangle rect, m::Vec3 color);

DebugLine* get_lines_for_frame(usize* out_count);

}
}


#endif // RIGEL_DEBUG_H
