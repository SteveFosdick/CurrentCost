#include "cc-common.h"
#include "file-logger.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

struct _file_logger_t {
    time_t switch_secs;
    FILE *xml_fp;
};

extern file_logger_t *file_logger_new(void)
{
    file_logger_t *file_logger;

    if ((file_logger = malloc(sizeof(file_logger_t)))) {
        file_logger->switch_secs = 0;
        file_logger->xml_fp = NULL;
        return file_logger;
    }
    return NULL;
}

extern void file_logger_free(file_logger_t * file_logger)
{
    if (file_logger) {
        if (file_logger->xml_fp)
            fclose(file_logger->xml_fp);
        free(file_logger);
    }
}

static void switch_file(file_logger_t * file_logger, time_t now_secs)
{
    struct tm *tp;
    FILE *nfp;
    char file[30];

    tp = gmtime(&now_secs);
    strftime(file, sizeof file, xml_file, tp);
    if ((nfp = fopen(file, "a"))) {
        if (file_logger->xml_fp != NULL)
            fclose(file_logger->xml_fp);
        file_logger->xml_fp = nfp;
        file_logger->switch_secs = now_secs + 86400 - (now_secs % 86400);
    }
    else
        log_syserr("unable to open file '%s' for append", file);
}

extern void file_logger_line(file_logger_t *file_logger, struct timespec *when, const char *line, const char *end)
{
    const char *ptr;
    FILE *fp;

    if ((ptr = strstr(line, "<msg>"))) {
        if (when->tv_sec >= file_logger->switch_secs)
            switch_file(file_logger, when->tv_sec);
        if ((fp = file_logger->xml_fp)) {
            ptr += 5;
            fwrite(line, ptr - line, 1, fp);
            fprintf(fp, "<host-tstamp>%lu.%06lu</host-tstamp>", when->tv_sec, when->tv_nsec / 1000);
            fwrite(ptr, end - ptr, 1, fp);
            fflush(fp);
        }
    }
    else {
        if ((fp = file_logger->xml_fp))
            fwrite(line, end - line, 1, fp);
    }
}
