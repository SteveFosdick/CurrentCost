#include "cc-common.h"
#include "mapfile.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

mf_status mapfile(const char *filename, void *user_data, mf_callback callback)
{

    mf_status status = MF_FAIL;
    int fd;
    struct stat stb;
    void *data;

    if ((fd = open(filename, O_RDONLY)) >= 0) {
        if (fstat(fd, &stb) == 0) {
            data = mmap(NULL, stb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
            if (data) {
                status = callback(user_data, data, stb.st_size);
                munmap(data, stb.st_size);
            } else
                log_syserr("unable to map file '%s' into memory", filename);
        } else
            log_syserr("unable to fstat '%s'", filename);
        close(fd);
    } else
        log_syserr("unable to open file '%s' for reading", filename);

    return status;
}
