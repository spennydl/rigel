#include "fs_linux.h"

#include <unistd.h>
#include <fcntl.h>

namespace rigel {

ubyte* slurp_into_mem(mem::Arena* dest, const char* file_name)
{
    int fd = open(file_name, O_RDONLY);
    if (fd < 0) {
        return nullptr;
    }

    i32 size_in_bytes = lseek(fd, 0, SEEK_END);
    if (size_in_bytes < 0) {
        close(fd);
        return nullptr;
    }
    lseek(fd, 0, SEEK_SET);

    ubyte* buffer = dest->alloc_array<ubyte>(size_in_bytes);
    i32 n_read = 0;
    while (n_read < size_in_bytes) {
        i32 this_read = read(fd, buffer + n_read, size_in_bytes - n_read);
        if (this_read <= 0) {
            break;
        }
        n_read += this_read;
    }
    close(fd);

    return buffer;
}

} // namespace rigel
