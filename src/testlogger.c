#include "cc-common.h"
#include "logger.h"

#include <unistd.h>

const char prog_name[] = "testlogger";

int main(int argc, char **argv)
{
    logger_t *l;
    unsigned char buffer[4096];
    ssize_t nbytes;

    if ((l = logger_new())) {
        while ((nbytes = read(0, buffer, sizeof buffer)) > 0)
            logger_data(l, buffer, nbytes);
        logger_free(l);
        return 0;
    }
    else {
        log_syserr("unable to allocate logger");
        return 1;
    }
}
