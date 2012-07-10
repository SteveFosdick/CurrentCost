#include "cc-common.h"
#include "parsefile.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define RE_TIM "([0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z)"
#define RE_NUM "([0-9\\.]+)"
#define RE_INT "([0-9]+)"

static const char pattern[] = "<host-tstamp>" RE_TIM ".*<tmpr>" RE_NUM ".*<sensor>" RE_INT ".*<watts>" RE_NUM;
#define MATCH_SIZE 5

static void log_reg_err(pf_context *ctx, int err, const char *what) {
    char buf[1024];

    regerror(err, &ctx->preg, buf, sizeof buf);
    log_msg("error %s regular expression - %s", what, buf);
}

pf_context *pf_new(void) {
    pf_context *ctx;
    int err;

    if ((ctx = malloc(sizeof(pf_context)))) {
	ctx->file_cb = pf_parse_forward;
	ctx->sample_cb = NULL;
	if ((err = regcomp(&ctx->preg, pattern, REG_EXTENDED)) == 0)
	    return ctx;
	else
	    log_reg_err(ctx, err, "compiling");
	free(ctx);
    } else
	log_syserr("allocate parse context");
    return ctx;
}

void pf_free(pf_context *ctx) {
    regfree(&ctx->preg);
    free(ctx);
}

static pf_status exec_regex(pf_context *ctx, char *line) {
    pf_status status = PF_SUCCESS;
    regmatch_t matches[MATCH_SIZE], *match;
    int err, size;
    pf_sample smp;

    if ((err = regexec(&ctx->preg, line, MATCH_SIZE, matches, 0)) == 0) {
	match = matches + 1;
	size = match->rm_eo - match->rm_so;
	if (size > sizeof smp.timestamp)
	    size = sizeof smp.timestamp;
	strncpy(smp.timestamp, line + match->rm_so, size);
	match++;
	smp.temp = strtod(line + match->rm_so, NULL);
	match++;
	smp.sensor = strtoul(line + match->rm_so, NULL, 10);
	match++;
	smp.watts = strtod(line + match->rm_so, NULL);
	status = ctx->sample_cb(ctx, &smp);
    } else if (err != REG_NOMATCH) {
	log_reg_err(ctx, err, "executing");
	status = PF_FAIL;
    }
    return status;
}

pf_status pf_parse_line(pf_context *ctx, const char *line, const char *end) {
    pf_status status = PF_SUCCESS;
    char *copy;
    size_t size;

    if (end > line) {
	size = end - line;
	copy = alloca(size + 1);
	memcpy(copy, line, size);
	copy[size] = '\0';
	status = exec_regex(ctx, copy);
    }
    return status;
}

pf_status pf_parse_forward(pf_context *ctx, void *data, size_t size) {
    pf_status status = PF_SUCCESS;
    const char *ptr = data;
    const char *end = ptr + size;
    const char *line = ptr;

    do {
	while (ptr < end && *ptr++ != '\n')
	    ;
	status = pf_parse_line(ctx, line, ptr-1);
	line = ptr;
    } while (status == PF_SUCCESS && ptr < end);

    return status;
}

pf_status pf_parse_backward(pf_context *ctx, void *data, size_t size) {
    pf_status status = PF_SUCCESS;
    const char *start = data;
    const char *ptr = start + size;
    const char *line = ptr;

    do {
	while (line > start && *--line != '\n')
	    ;
	status = pf_parse_line(ctx, line+1, ptr);
	ptr = line;
    } while (status == PF_SUCCESS && ptr > start);

    return status;
}

pf_status pf_parse_file(pf_context *ctx, const char *filename) {
    pf_status status = PF_FAIL;
    int fd;
    struct stat stb;
    void *data;

    if ((fd = open(filename, O_RDONLY)) >= 0) {
	if (fstat(fd, &stb) == 0) {
	    if ((data = mmap(NULL, stb.st_size, PROT_READ, MAP_PRIVATE, fd, 0)))
		status = ctx->file_cb(ctx, data, stb.st_size);
	    else
		log_syserr("unable to map file '%s' into memory", filename);
	}
	else
	    log_syserr("unable to fstat '%s'", filename);
    }
    else
	log_syserr("unable to open file '%s' for reading", filename);

    return status;
}
