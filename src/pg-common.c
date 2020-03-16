#include "cc-common.h"
#include "pg-common.h"

#include <stdarg.h>
#include <string.h>
#include <time.h>

const char power_sql[] =
    "INSERT INTO power (time_stamp, sensor, id, temperature, watts) "
    "VALUES ($1, $2, $3, $4, $5)";

const char pulse_sql[] =
    "INSERT INTO pulse (time_stamp, sensor, id, temperature, pulses) "
    "VALUES ($1, $2, $3, $4, $5)";

void log_db_err(PGconn *conn, const char *msg, ...)
{
    va_list ap;
    struct timespec tv;
    struct tm *tp;
    char stamp[24];
    const char *ptr, *end;

    clock_gettime(CLOCK_REALTIME, &tv);
    tp = localtime(&tv.tv_sec);
    strftime(stamp, sizeof stamp, log_hdr1, tp);
    fprintf(stderr, log_hdr2, stamp, (int) (tv.tv_nsec / 1000000), prog_name);
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    putc('\n', stderr);
    va_end(ap);
    ptr = PQerrorMessage(conn);
    while ((end = strchr(ptr, '\n'))) {
        fprintf(stderr, log_hdr2, stamp, (int) (tv.tv_nsec / 1000000), prog_name);
        fwrite(ptr, end - ptr + 1, 1, stderr);
        ptr = end + 1;
    }
}

static const char *find_tok(const char *pat, const char *str, const char *end)
{
    const char *pat_ptr, *str_ptr;
    int ch;

    while (str < end) {
        str_ptr = str;
        pat_ptr = pat;
        ch = *pat_ptr++;
        while (str_ptr < end && ch == *str_ptr) {
            ch = *pat_ptr++;
            str_ptr++;
        }
        if (!ch)
            return str_ptr;
        str++;
    }
    return NULL;
}

const char *parse_int(sample_t *smp, int index, const char *pat, const char *str, const char *end)
{
    const char *src_ptr;
    char *dst_start, *dst_ptr, *dst_max;
    int ch;
    size_t len;

    if ((src_ptr = find_tok(pat, str, end))) {
        dst_ptr = dst_start = smp->ptr.data_ptr;
        dst_max = smp->data + MAX_DATA;
        while (src_ptr < end && dst_ptr < dst_max) {
            ch = *src_ptr++;
            if (ch >= '0' && ch <= '9')
                *dst_ptr++ = ch;
            else if (ch == '<') {
                len = dst_ptr - dst_start;
                if (len > 0) {
                    *dst_ptr++ = '\0';
                    smp->values[index] = dst_start;
                    smp->lengths[index] = len;
                    smp->ptr.data_ptr = dst_ptr;
                    return src_ptr;
                }
            }
            else
                break;
        }
    }
    return NULL;
}

const char *parse_real(sample_t *smp, int index, const char *pat, const char *str, const char *end)
{
    const char *src_ptr;
    char *dst_start, *dst_ptr, *dst_max;
    int ch;
    size_t len;

    if ((src_ptr = find_tok(pat, str, end))) {
        dst_ptr = dst_start = smp->ptr.data_ptr;
        dst_max = smp->data + MAX_DATA;
        while (src_ptr < end && dst_ptr < dst_max) {
            ch = *src_ptr++;
            if ((ch >= '0' && ch <= '9') || ch == '.')
                *dst_ptr++ = ch;
            else if (ch == '<') {
                len = dst_ptr - dst_start;
                if (len > 0) {
                    *dst_ptr++ = '\0';
                    smp->values[index] = dst_start;
                    smp->lengths[index] = len;
                    smp->ptr.data_ptr = dst_ptr;
                    return src_ptr;
                }
            }
            else
                break;
        }
    }
    return NULL;
}

