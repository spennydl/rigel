#ifndef RIGEL_FS_LINUX_H
#define RIGEL_FS_LINUX_H

#include "rigel.h"
#include "mem.h"

namespace rigel {

ubyte* slurp_into_mem(mem::Arena* dest, const char* file_name);

} // namespace rigel

#endif // RIGEL_FS_LINUX_H
