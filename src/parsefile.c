#include "cc-common.h"
#include "parsefile.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

pf_context *pf_new(void) {
    pf_context *ctx;
    pf_pulse *ptr, *end;

    if ((ctx = malloc(sizeof(pf_context)))) {
	ctx->file_cb = tf_parse_cb_forward;
	ctx->filter_cb = pf_filter_all;
	ctx->sample_cb = NULL;
        ptr = ctx->pulse;
        end = ptr + MAX_SENSOR;
        while (ptr < end) {
            ptr->timestamp = 0;
            ptr->count = 0;
            ptr->ipu = 1000;
            ptr++;
        }
        return ctx;
    } else
	log_syserr("allocate parse context");
    return ctx;
}

void pf_free(pf_context *ctx) {
    free(ctx);
}

mf_status pf_filter_all(pf_context *ctx, time_t ts) {
    return MF_SUCCESS;
}

static mf_status pulse_sample(pf_context *ctx, char *tail, pf_sample *smp) {
    mf_status status = MF_SUCCESS;
    char *ptr, *end;
    time_t ts_new, ts_diff;
    long count_old, count_new, count_diff, ipu;
    pf_pulse *pulse;
    double watts;

    printf("pulse sensor: %d\n", smp->sensor);

    ts_new = smp->timestamp;
    count_new = strtoul(tail, &end, 10);
    ptr = end;
    if (*ptr == '<') {
        pulse = ctx->pulse + smp->sensor;
        if ((count_old = pulse->count) >= 0) {
            if ((ptr = strstr(end, "<ipu>"))) {
                ipu = strtoul(ptr + 5, NULL, 10);
                if (ipu == 0)
                    ipu = pulse->ipu;
                pulse->ipu = ipu;
                ts_diff = ts_new - pulse->timestamp;
                smp->timestamp = ts_new + (ts_diff / 2);
                if (ts_diff < 0)
                    ts_diff = -ts_diff;
                count_diff = count_new - count_old;
                if (count_diff < 0)
                    count_diff = -count_diff;
                watts = (double)count_diff * 3600000 / (ts_diff * ipu);
                if (watts >= 0 && watts <= 25000) {
                    smp->watts = watts;
                    status = ctx->sample_cb(ctx, smp);
                }
                else
                    printf("calc error: ts_new=%ld, watts=%g, time_diff=%ld, count_diff=%ld, count_old=%ld, count_new=%ld, ipu=%ld\n", ts_new, watts, ts_diff, count_diff, count_old, count_new, ipu);
            }
        }
        pulse->timestamp = ts_new;
        pulse->count = count_new;
    }
    return status;
}

static mf_status sensor_search(pf_context *ctx, char *tail, time_t ts_secs) {
    mf_status status = MF_SUCCESS;
    char *ptr, *end;
    int sensor;
    pf_sample smp;

    if ((ptr = strstr(tail, "<tmpr>"))) {
        smp.temp = strtod(ptr + 6, &end);
        ptr = end;
        if (*ptr == '<') {
            if ((ptr = strstr(ptr, "<sensor>"))) {
                sensor = strtoul(ptr + 8, &end, 10);
                ptr = end;
                if (*ptr == '<' && sensor >= 0 && sensor < MAX_SENSOR) {
                    smp.sensor = sensor;
                    smp.timestamp = ts_secs;
                    if ((ptr = strstr(ptr, "<watts>"))) {
                        smp.watts = strtod(ptr + 7, &end);
                        if (*end == '<')
                            status = ctx->sample_cb(ctx, &smp);
                    } else if ((ptr = strstr(end, "<imp>")))
                        status = pulse_sample(ctx, ptr + 5, &smp);
                 }
            }
        }
    }
    return status;
}

static mf_status tstamp_search(pf_context *ctx, char *line) {
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
			const void *file_data, size_t file_size) {

    mf_status status = MF_SUCCESS;
    char *copy;

    if (file_size > 135) {
	copy = alloca(file_size + 1);
	memcpy(copy, file_data, file_size);
	copy[file_size] = '\0';
        status = tstamp_search((pf_context *)user_data, copy);
    }
    return status;
}
