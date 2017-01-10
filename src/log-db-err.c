#include "cc-common.h"
#include "log-db-err.h"

#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

void log_db_err(PGconn *conn, const char *msg, ...) {
    va_list ap;
    struct timeval tv;
    struct tm *tp;
    char stamp[24];
    const char *ptr, *end;

    gettimeofday(&tv, NULL);
    tp = localtime(&tv.tv_sec);
    strftime(stamp, sizeof stamp, log_hdr1, tp);
    fprintf(stderr, log_hdr2, stamp, (int) (tv.tv_usec / 1000), prog_name);
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    putc('\n', stderr);
    va_end(ap);
    ptr = PQerrorMessage(conn);
    while ((end = strchr(ptr, '\n'))) {
	fprintf(stderr, log_hdr2, stamp, (int) (tv.tv_usec / 1000), prog_name);
	fwrite(ptr, end - ptr + 1, 1, stderr);
	ptr = end + 1;
    }
}

