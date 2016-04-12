#include "cc-common.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#ifndef DEFAULT_DIR
#define DEFAULT_DIR "/share/fozzy/Data/CurrentCost"
#endif

const char default_dir[] = DEFAULT_DIR;
const char xml_file[] = "cc-%Y-%m-%d.xml";
const char date_iso[] = "%Y-%m-%dT%H:%M:%SZ";

static void log_common(const char *msg, va_list ap)
{
    struct timeval tv;
    struct tm *tp;
    char stamp[24];

    gettimeofday(&tv, NULL);
    tp = localtime(&tv.tv_sec);
    strftime(stamp, sizeof stamp, "%d/%m/%Y %H:%M:%S", tp);
    fprintf(stderr, "%s.%03d %s: ", stamp, (int) (tv.tv_usec / 1000),
            prog_name);
    vfprintf(stderr, msg, ap);
}

void log_msg(const char *msg, ...)
{
    va_list ap;

    va_start(ap, msg);
    log_common(msg, ap);
    putc('\n', stderr);
    va_end(ap);
}

void log_syserr(const char *msg, ...)
{
    const char *syserr;
    va_list ap;

    syserr = strerror(errno);
    va_start(ap, msg);
    log_common(msg, ap);
    fprintf(stderr, " - %s\n", syserr);
}
