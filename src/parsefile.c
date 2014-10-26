#include "cc-common.h"
#include "parsefile.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

pf_context *pf_new(void)
{
    pf_context *ctx;

    if ((ctx = malloc(sizeof(pf_context)))) {
        ctx->file_cb = tf_parse_cb_forward;
        ctx->filter_cb = pf_filter_all;
        ctx->sample_cb = NULL;
        ctx->pulse_cb = NULL;
        return ctx;
    } else
        log_syserr("allocate parse context");
    return ctx;
}

void pf_free(pf_context * ctx)
{
    free(ctx);
}

mf_status pf_filter_all(pf_context * ctx, time_t ts)
{
    return MF_SUCCESS;
}

static mf_status sensor_search(pf_context * ctx, char *tail, time_t ts_secs)
{
    mf_status status = MF_SUCCESS;
    char *ptr, *end;
    pf_sample smp;

    if ((ptr = strstr(tail, "<tmpr>"))) {
        smp.temp = strtod(ptr + 6, &end);
        ptr = end;
        if (*ptr == '<') {
            if ((ptr = strstr(ptr, "<sensor>"))) {
                smp.sensor = strtoul(ptr + 8, &end, 10);
                ptr = end;
                if (*ptr == '<') {
                    smp.timestamp = ts_secs;
                    if ((ptr = strstr(ptr, "<watts>"))) {
                        smp.data.watts = strtod(ptr + 7, &end);
                        if (*end == '<')
                            status = ctx->sample_cb(ctx, &smp);
                    } else if ((ptr = strstr(end, "<imp>"))) {
                        smp.data.pulse.count = strtoul(ptr + 5, &end, 10);
                        if (*end == '<') {
                            if ((ptr = strstr(end, "<ipu>"))) {
                                smp.data.pulse.ipu = strtoul(ptr + 5, NULL, 10);
                                status = ctx->pulse_cb(ctx, &smp);
                            }
                        }
                    }
                }
            }
        }
    }
    return status;
}

static mf_status tstamp_search(pf_context * ctx, char *line)
{
    mf_status status = MF_SUCCESS;
    time_t ts_secs;
    char *ptr, *end;

    if ((ptr = strstr(line, "<host-tstamp>"))) {
        ts_secs = strtoul(ptr + 13, &end, 10);
        if (*end == '<') {
            if ((status = ctx->filter_cb(ctx, ts_secs)) == MF_SUCCESS)
                status = sensor_search(ctx, end, ts_secs);
            else if (status == MF_IGNORE)
                status = MF_SUCCESS;
        }
    }
    return status;
}

mf_status pf_parse_line(void *user_data,
                        const void *file_data, size_t file_size)
{

    mf_status status = MF_SUCCESS;
    char *copy;

    if (file_size > 135) {
        copy = alloca(file_size + 1);
        memcpy(copy, file_data, file_size);
        copy[file_size] = '\0';
        status = tstamp_search((pf_context *) user_data, copy);
    }
    return status;
}
