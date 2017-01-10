#include "cc-defs.h"
#include "cc-common.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

const char default_dir[] = DEFAULT_DIR;
const char xml_file[]    = XML_FILE;
const char date_iso[]    = DATE_ISO;

const char log_hdr1[] = "%d/%m/%Y %H:%M:%S";
const char log_hdr2[] = "%s.%03d %s: ";

static void log_common(const char *msg, va_list ap)
{
    struct timeval tv;
    struct tm *tp;
    char stamp[24];

    gettimeofday(&tv, NULL);
    tp = localtime(&tv.tv_sec);
    strftime(stamp, sizeof stamp, log_hdr1, tp);
    fprintf(stderr, log_hdr2, stamp, (int) (tv.tv_usec / 1000), prog_name);
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
