#ifndef RIGEL_FS_LINUX_H
#define RIGEL_FS_LINUX_H

#include <stdio.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

#include "rigel.h"
#include "mem.h"

namespace rigel {

ubyte* slurp_into_mem(mem::Arena* dest, const char* file_name);

namespace fs
{

struct File
{
    FILE* p;
};

struct Directory
{
    DIR* d;
};

Directory
open_dir(const char* dir);
b32 
extension_equals(const char* filename, const char* ext);

template <typename OpFn>
void for_each_file(Directory d, OpFn operation)
{
    struct dirent* dir;

    while (true)
    {
        errno = 0;
        dir = readdir(d.d);
        if (dir == nullptr)
        {
            assert(errno == 0 && "Directory error");
            break;
        }

        if (dir->d_type == DT_REG)
        {
            operation(dir->d_name);
        }
    }
}

} // namespace fs
} // namespace rigel

#endif // RIGEL_FS_LINUX_H
