#include "cc-common.h"
#include "history.h"
#include "parsefile.h"

#include <stdlib.h>

static void init_data(hist_context * ctx, time_t from, time_t to)
{
    hist_point *point;
    hist_sensor *sens, *sens_end;
    int sens_num;

    for (point = ctx->data; point < ctx->end; point++) {
        point->total = -1;
        point->others = -1;
        point->temp_total = 0;
        point->temp_count = 0;
        point->temp_mean = -1;
        sens = point->sensors;
        for (sens_end = sens + MAX_SENSOR; sens < sens_end; sens++) {
            sens->mean = -1;
            sens->total = 0;
            sens->count = 0;
            sens->pulse_tstamp_min = to;
            sens->pulse_tstamp_max = from;
            sens->pulse_count_min = 0;
            sens->pulse_count_max = 0;
        }
    }
    for (sens_num = 0; sens_num < MAX_SENSOR; sens_num++) {
        ctx->flags[sens_num] = 0;
        ctx->pulse_ipu[sens_num] = 0;
    }
}

static mf_status sample_cb(pf_context * pf, pf_sample * smp)
{
    hist_context *ctx = pf->user_data;
    hist_point *point;
    hist_sensor *sens_ptr;
    int sens_num;

    if (smp->timestamp >= ctx->start_ts && smp->timestamp < ctx->end_ts) {
        point = ctx->data + ((smp->timestamp - ctx->start_ts) / ctx->step);
        if (point < ctx->end) {
            point->temp_total += smp->temp;
            point->temp_count++;
            sens_num = smp->sensor;
            if (sens_num >= 0 && sens_num < MAX_SENSOR) {
                sens_ptr = point->sensors + sens_num;
                sens_ptr->total += smp->data.watts;
                sens_ptr->count++;
                ctx->flags[sens_num] = 'W';
            }
        } else
            log_msg("watt point with timestamp %lu is too big",
                    smp->timestamp);
    }
    return MF_SUCCESS;
}

static mf_status pulse_cb(pf_context * pf, pf_sample * smp)
{
    hist_context *ctx = pf->user_data;
    hist_point *point;
    hist_sensor *sens_ptr;
    time_t timestamp;
    int sens_num;

    timestamp = smp->timestamp;
    if (timestamp >= ctx->start_ts && timestamp < ctx->end_ts) {
        point = ctx->data + ((timestamp - ctx->start_ts) / ctx->step);
        if (point < ctx->end) {
            point->temp_total += smp->temp;
            point->temp_count++;
            sens_num = smp->sensor;
            if (sens_num >= 0 && sens_num < MAX_SENSOR) {
                sens_ptr = point->sensors + sens_num;
                if (timestamp < sens_ptr->pulse_tstamp_min) {
                    sens_ptr->pulse_tstamp_min = timestamp;
                    sens_ptr->pulse_count_min = smp->data.pulse.count;
                }
                if (timestamp > sens_ptr->pulse_tstamp_max) {
                    sens_ptr->pulse_tstamp_max = timestamp;
                    sens_ptr->pulse_count_max = smp->data.pulse.count;
                }
                ctx->flags[sens_num] = 'P';
                if (ctx->pulse_ipu[sens_num] == 0) {
                    ctx->pulse_ipu[sens_num] = smp->data.pulse.ipu;
                }
            }
        } else
            log_msg("pulse point with timestamp %lu is too big",
                    smp->timestamp);
    }
    return MF_SUCCESS;
}

static mf_status filter_cb_forw(pf_context * pf, time_t ts)
{
    hist_context *ctx = pf->user_data;

    if (ts < ctx->start_ts)
        return MF_IGNORE;
    if (ts >= ctx->end_ts)
        return MF_STOP;
    return MF_SUCCESS;
}

static mf_status filter_cb_back(pf_context * pf, time_t ts)
{
    hist_context *ctx = pf->user_data;

    if (ts < ctx->start_ts)
        return MF_STOP;
    if (ts >= ctx->end_ts)
        return MF_IGNORE;
    return MF_SUCCESS;
}

static inline int same_day(struct tm *a, struct tm *b)
{
    return a->tm_mday == b->tm_mday
        && a->tm_mon == b->tm_mon && a->tm_year == b->tm_year;
}

#define SECS_IN_DAY (24 * 60 * 60)

static mf_status fetch_data(hist_context * ctx)
{
    mf_status status = MF_FAIL;
    pf_context *pf;
    time_t now, ts;
    struct tm tm_now, tm_ts;
    int mid;
    char file[30];

    if ((pf = pf_new())) {
        pf->filter_cb = filter_cb_forw;
        pf->sample_cb = sample_cb;
        pf->pulse_cb = pulse_cb;
        pf->user_data = ctx;
        time(&now);
        gmtime_r(&now, &tm_now);
        gmtime_r(&ctx->start_ts, &tm_ts);
        mid = 12;
        if (same_day(&tm_now, &tm_ts))
            mid = tm_now.tm_hour / 2;
        if (tm_ts.tm_hour > mid) {
            pf->file_cb = tf_parse_cb_backward;
            pf->filter_cb = filter_cb_back;
            log_msg("initial file to be read backwards");
        }
        strftime(file, sizeof file, xml_file, &tm_ts);
        log_msg("read file '%s'", file);
        if (pf_parse_file(pf, file) != MF_FAIL) {
            pf->file_cb = tf_parse_cb_forward;
            pf->filter_cb = filter_cb_forw;
            status = MF_SUCCESS;
            ts = ctx->start_ts;
            ts += SECS_IN_DAY - (ts % SECS_IN_DAY);
            for (; ts < ctx->end_ts; ts += SECS_IN_DAY) {
                gmtime_r(&ts, &tm_ts);
                strftime(file, sizeof file, xml_file, &tm_ts);
                log_msg("read file '%s'", file);
                if (pf_parse_file(pf, file) == MF_FAIL) {
                    status = MF_FAIL;
                    break;
                }
            }
        }
        pf_free(pf);
    }
    return status;
}

static double pulse_calc(hist_sensor * sens, int ipu)
{
    time_t ts_diff;
    long count_diff;
    double watts;

    ts_diff = sens->pulse_tstamp_max - sens->pulse_tstamp_min;
    if (ts_diff > 0) {
        count_diff = sens->pulse_count_max - sens->pulse_count_min;
        watts = (double) count_diff *3600000 / (ts_diff * ipu);
        if (watts >= 0 && watts <= 10000)
            return watts;
    }
    return -1;
}

static void crunch_data(hist_context * ctx)
{
    hist_point *point;
    hist_sensor *sens;
    int sens_num, ipu;
    double apps, total, value;

    for (point = ctx->data; point < ctx->end; point++) {
        apps = 0.0;
        for (sens_num = 0; sens_num < MAX_SENSOR; sens_num++) {
            sens = point->sensors + sens_num;
            if ((ipu = ctx->pulse_ipu[sens_num]) > 0)
                value = pulse_calc(sens, ipu);
            else {
                value = -1;
                if (sens->count > 0)
                    value = sens->total / sens->count;
            }
            sens->mean = value;
            if (sens_num >= 1 && sens_num <= 5) // if applicance monitor.
                apps += value;
        }
        if (point->sensors[9].mean > 0)
            total = point->sensors[8].mean + point->sensors[9].mean;
        else
            total = point->sensors[8].mean - point->sensors[0].mean;
        point->total = total;
        point->others = total - apps;
        if (point->temp_count > 0)
            point->temp_mean = point->temp_total / point->temp_count;
    }
}

hist_context *hist_get(time_t from, time_t to, int step)
{
    hist_context *ctx;
    int points;

    if ((ctx = malloc(sizeof(hist_context)))) {
        ctx->start_ts = from;
        ctx->end_ts = to;
        ctx->step = step;
        points = (to - from) / step + 1;
        if ((ctx->data = malloc(points * sizeof(hist_point)))) {
            ctx->end = ctx->data + points;
            init_data(ctx, from, to);
            if (fetch_data(ctx) == MF_SUCCESS) {
                crunch_data(ctx);
                return ctx;
            }
        } else
            log_syserr("unable to allocate space for history points");
        free(ctx);
    } else
        log_syserr("unable to allocate space for history context");
    return NULL;
}

void hist_free(hist_context * ctx)
{
    free(ctx->data);
    free(ctx);
}

void hist_js_temp_out(hist_context * ctx, FILE * ofp)
{
    hist_point *point;
    double cur_value = 0.0, new_value;
    int ch = '[';

    for (point = ctx->data; point < ctx->end; point++) {
        if ((new_value = point->temp_mean) >= 0)
            cur_value = new_value;
        fprintf(ofp, "%c%.3g", ch, cur_value);
        ch = ',';
    }
    putc(']', ofp);
}

void hist_js_sens_out(hist_context * ctx, FILE * ofp, int sensor)
{
    hist_point *point;
    double cur_value = 0.0, new_value;
    int ch;

    if (sensor >= 0 && sensor < MAX_SENSOR) {
        if (ctx->flags[sensor]) {
            ch = '[';
            for (point = ctx->data; point < ctx->end; point++) {
                if ((new_value = point->sensors[sensor].mean) >= 0)
                    cur_value = new_value;
                fprintf(ofp, "%c%g", ch, cur_value);
                ch = ',';
            }
            putc(']', ofp);
        }
    }
}

void hist_js_total_out(hist_context * ctx, FILE * ofp)
{
    hist_point *point;
    double cur_value = 0.0, new_value;
    int ch = '[';

    for (point = ctx->data; point < ctx->end; point++) {
        if ((new_value = point->total) >= 0)
            cur_value = new_value;
        fprintf(ofp, "%c%.3g", ch, cur_value);
        ch = ',';
    }
    putc(']', ofp);
}

void hist_js_others_out(hist_context * ctx, FILE * ofp)
{
    hist_point *point;
    double cur_value = 0.0, new_value;
    int ch = '[';

    for (point = ctx->data; point < ctx->end; point++) {
        if ((new_value = point->others) >= 0)
            cur_value = new_value;
        fprintf(ofp, "%c%.3g", ch, cur_value);
        ch = ',';
    }
    putc(']', ofp);
}
