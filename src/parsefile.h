#ifndef PARSEFILE_H
#define PARSEFILE_H

#include "textfile.h"

#include <regex.h>
#include <time.h>

typedef struct _pf_pulse {
    long count;
    int  ipu;
} pf_pulse;

typedef struct _pf_sample {
    time_t timestamp;
    double temp;
    int    sensor;
    union {
        double   watts;
        pf_pulse pulse;
    } data;
} pf_sample;

typedef struct _pf_context pf_context;

typedef mf_status (*pf_filter_cb)(pf_context *ctx, time_t ts);
typedef mf_status (*pf_sample_cb)(pf_context *ctx, pf_sample *smp);

typedef struct _prev_pulse {
    time_t       timestamp;
    long         count;
    int          ipu;
} prev_pulse_t;

struct _pf_context {
    mf_callback  file_cb;
    pf_filter_cb filter_cb;
    pf_sample_cb sample_cb;
    pf_sample_cb pulse_cb;
    void         *user_data;
    prev_pulse_t prev_pulses[MAX_SENSOR];
};

extern pf_context *pf_new(void);
extern void pf_free(pf_context *ctx);

extern mf_status pf_filter_all(pf_context *ctx, time_t ts);
extern mf_status pf_default_pulse_cb(pf_context *ctx, pf_sample *smp);
extern mf_status pf_parse_line(void *user_data,
			       const void *file_data, size_t file_size);

#define pf_parse_file(ctx, file) \
    tf_parse_file(file, ctx, ctx->file_cb, pf_parse_line)
#endif
