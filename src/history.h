#ifndef HISTORY_H
#define HISTORY_H

#include <stdio.h>
#include <time.h>

typedef struct _sensor {
    float  mean;
    float  total;
    int    count;
    time_t pulse_tstamp_min;
    time_t pulse_tstamp_max;
    long   pulse_count_min;
    long   pulse_count_max;
} hist_sensor;

typedef struct _point {
    float  total;
    float  others;
    float  temp_total;
    int    temp_count;
    float  temp_mean;
    hist_sensor sensors[MAX_SENSOR];
} hist_point;

typedef struct _hist_context {
    time_t start_ts;
    time_t end_ts;
    time_t step;
    hist_point *data;
    hist_point *end;
    char flags[MAX_SENSOR];
    int  pulse_ipu[MAX_SENSOR];
} hist_context;

typedef enum {
    HIST_SUCCESS,
    HIST_FAIL
} hist_status;

extern hist_context *hist_get(time_t from, time_t to, int step);
extern void hist_free(hist_context *ctx);

extern void hist_js_temp_out(hist_context *ctx, FILE *ofp);
extern void hist_js_sens_out(hist_context *ctx, FILE *ofp, int sensor);
extern void hist_js_total_out(hist_context *ctx, FILE *ofp);
extern void hist_js_others_out(hist_context *ctx, FILE *ofp);
#endif
