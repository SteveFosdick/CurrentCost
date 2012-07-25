#include "cc-common.h"
#include "parsefile.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

static const char timestamp_pat[] =
    "<host-tstamp>([0-9]+)</host-tstamp>";

#define TIMESTAMP_NMATCH 2

#define RE_NUM "([0-9\\.]+)"
#define RE_INT "([0-9]+)"

static const char sample_pat[] =
    "<tmpr>" RE_NUM
    ".*<sensor>" RE_INT
    ".*<watts>" RE_NUM;

#define SAMPLE_NMATCH 4

static void log_reg_err(regex_t *preg, int err, const char *what) {
    char buf[1024];

    regerror(err, preg, buf, sizeof buf);
    log_msg("error %s regular expression - %s", what, buf);
}

pf_context *pf_new(void) {
    pf_context *ctx;
    int err;

    if ((ctx = malloc(sizeof(pf_context)))) {
	ctx->file_cb = tf_parse_cb_forward;
	ctx->filter_cb = pf_filter_all;
	ctx->sample_cb = NULL;
	if ((err = regcomp(&ctx->timestamp_re, timestamp_pat, REG_EXTENDED)) == 0) {
	    if ((err = regcomp(&ctx->sample_re, sample_pat, REG_EXTENDED)) == 0)
		return ctx;
	    log_reg_err(&ctx->sample_re, err, "compiling samples");
	    regfree(&ctx->timestamp_re);
	}
	log_reg_err(&ctx->timestamp_re, err, "compiling timestamp");
	free(ctx);
    } else
	log_syserr("allocate parse context");
    return ctx;
}

void pf_free(pf_context *ctx) {
    regfree(&ctx->sample_re);
    regfree(&ctx->timestamp_re);
    free(ctx);
}

mf_status pf_filter_all(pf_context *ctx, time_t ts) {
    return MF_SUCCESS;
}

static mf_status exec_sample_re(pf_context *ctx, char *tail, time_t ts_secs) {
    mf_status status = MF_SUCCESS;
    regmatch_t matches[SAMPLE_NMATCH], *match;
    int err;
    pf_sample smp;
    
    if ((err = regexec(&ctx->sample_re, tail, SAMPLE_NMATCH, matches, 0)) == 0) {
	match = matches + 1;
	smp.timestamp = ts_secs;
	smp.temp = strtod(tail + match->rm_so, NULL);
	match++;
	smp.sensor = strtoul(tail + match->rm_so, NULL, 10);
	match++;
	smp.watts = strtod(tail + match->rm_so, NULL);
	status = ctx->sample_cb(ctx, &smp);
    } else if (err != REG_NOMATCH) {
	log_reg_err(&ctx->sample_re, err, "executing sample");
	status = MF_FAIL;
    }
    return status;
}

static mf_status exec_timestamp_re(pf_context *ctx, char *line) {
    mf_status status = MF_SUCCESS;
    regmatch_t matches[TIMESTAMP_NMATCH];
    int err;
    time_t ts_secs;

    if ((err = regexec(&ctx->timestamp_re, line, TIMESTAMP_NMATCH, matches, 0)) == 0) {
	ts_secs = strtoul(line + matches[1].rm_so, NULL, 10);
	if ((status = ctx->filter_cb(ctx, ts_secs)) == MF_SUCCESS)
	    status = exec_sample_re(ctx, line + matches[0].rm_eo, ts_secs);
	else if (status == MF_IGNORE)
	    status = MF_SUCCESS;
    } else if (err != REG_NOMATCH) {
	log_reg_err(&ctx->timestamp_re, err, "executing timestamp");
	status = MF_FAIL;
    }
    return status;
}

mf_status pf_parse_line(void *user_data,
			const void *file_data, size_t file_size) {

    mf_status status = MF_SUCCESS;
    char *copy;

    if (file_size > 0) {
	copy = alloca(file_size + 1);
	memcpy(copy, file_data, file_size);
	copy[file_size] = '\0';
	status = exec_timestamp_re((pf_context *)user_data, copy);
    }
    return status;
}
