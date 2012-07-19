#include "cc-common.h"
#include "history.h"
#include "parsefile.h"

#include <stdlib.h>

static void init_data(hist_context *ctx) {
    hist_point *point;
    int i;

    for (point = ctx->data; point < ctx->end; point++) {
	for (i = 0; i < MAX_SENSOR; i++) {
	    point->totals[i] = 0;
	    point->counts[i] = 0;
	}
	point->temp_total = 0;
	point->temp_count = 0;
    }
    for (i = 0; i < MAX_SENSOR; i++)
	ctx->flags[i] = 0;
}

static pf_status sample_cb(pf_context *pf, pf_sample *smp) {
    hist_context *ctx = pf->user_data;
    hist_point *point;
    int sensor;

    if (smp->timestamp >= ctx->start_ts && smp->timestamp < ctx->end_ts) {
	point = ctx->data + ((smp->timestamp - ctx->start_ts) / ctx->step);
	if (point < ctx->end) {
	    point->temp_total += smp->temp;
	    point->temp_count++;
	    sensor = smp->sensor;
	    if (sensor >= 0 && sensor < MAX_SENSOR) {
		point->totals[sensor] += smp->watts;
		point->counts[sensor]++;
		ctx->flags[sensor] = 1;
	    }
	} else
	    log_msg("point with timestamp %lu is too big", smp->timestamp);
    }
    return PF_SUCCESS;
}

static pf_status filter_cb_forw(pf_context *pf, time_t ts) {
    hist_context *ctx = pf->user_data;
    if (ts >= ctx->end_ts)
	return PF_STOP;
    return PF_SUCCESS;
}

static pf_status filter_cb_back(pf_context *pf, time_t ts) {
    hist_context *ctx = pf->user_data;
    char tmstr[20];

    if (ts < ctx->start_ts) {
	strftime(tmstr, sizeof tmstr, date_iso, gmtime(&ts));
	log_msg("stop at %s", tmstr);
	return PF_STOP;
    }
    return PF_SUCCESS;
}

static inline int same_day(struct tm *a, struct tm *b) {
    return a->tm_mday == b->tm_mday
	&& a->tm_mon  == b->tm_mon
	&& a->tm_year  == b->tm_year;
}

#define SECS_IN_DAY (24 * 60 * 60)

static pf_status fetch_data(hist_context *ctx) {
    pf_status status = PF_FAIL;
    pf_context *pf;
    time_t now, ts;
    struct tm tm_now, tm_ts;
    int mid;
    char file[30];

    if ((pf = pf_new())) {
	pf->filter_cb = filter_cb_forw;
	pf->sample_cb = sample_cb;
	pf->user_data = ctx;
	time(&now);
	gmtime_r(&now, &tm_now);
	gmtime_r(&ctx->start_ts, &tm_ts);
	mid = 12;
	if (same_day(&tm_now, &tm_ts))
	    mid = tm_now.tm_hour / 2;
	if (tm_ts.tm_hour > mid) {
	    pf->file_cb = pf_parse_backward;
	    pf->filter_cb = filter_cb_back;
	    log_msg("initial file to be read backwards");
	}
	strftime(file, sizeof file, xml_file, &tm_ts);
	log_msg("read file '%s'", file);
	if (pf_parse_file(pf, file) != PF_FAIL) {
	    pf->file_cb = pf_parse_forward;
	    pf->filter_cb = filter_cb_forw;
	    status = PF_SUCCESS;
	    ts = ctx->start_ts;
	    ts += SECS_IN_DAY - (ts % SECS_IN_DAY);
	    for (; ts < ctx->end_ts; ts += SECS_IN_DAY) {
		gmtime_r(&ts, &tm_ts);
		strftime(file, sizeof file, xml_file, &tm_ts);
		log_msg("read file '%s'", file);
		if (pf_parse_file(pf, file) == PF_FAIL) {
		    status = PF_FAIL;
		    break;
		}
	    }
	}
	pf_free(pf);
    }
    return status;
}

hist_context *hist_get(time_t from, time_t to, int step) {
    hist_context *ctx;
    int points;

    if ((ctx = malloc(sizeof(hist_context)))) {
	ctx->start_ts = from;
	ctx->end_ts = to;
	ctx->step = step;
	points = (to - from) / step + 1;
	if ((ctx->data = malloc(points * sizeof(hist_point)))) {
	    ctx->end = ctx->data + points;
	    init_data(ctx);
	    if (fetch_data(ctx) == PF_SUCCESS)
		return ctx;
	} else
	    log_syserr("unable to allocate space for history points");
	free(ctx);
    } else
	log_syserr("unable to allocate space for history context");
    return NULL;
}

void hist_free(hist_context *ctx) {
    free(ctx->data);
    free(ctx);
}

void hist_js_temp_out(hist_context *ctx, FILE *ofp) {
    hist_point *point;
    double value = 0.0;
    int ch = '[';

    for (point = ctx->data; point < ctx->end; point++) {
	if (point->temp_count > 0)
	    value = point->temp_total / point->temp_count;
	fprintf(ofp, "%c%g", ch, value);
	ch = ',';
    }
    putc(']', ofp);
}

void hist_js_sens_out(hist_context *ctx, FILE *ofp, int sensor) {
    hist_point *point;
    double value;
    int ch;

    if (sensor >= 0 && sensor < MAX_SENSOR) {
	if (ctx->flags[sensor]) {
	    ch = '[';
	    value = 0.0;
	    for (point = ctx->data; point < ctx->end; point++) {
		if (point->counts[sensor] > 0)
		    value = point->totals[sensor] / point->counts[sensor];
		fprintf(ofp, "%c%g", ch, value);
		ch = ',';
	    }
	    putc(']', ofp);
	}
    }
}
