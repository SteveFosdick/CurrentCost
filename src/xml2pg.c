#include "cc-defs.h"
#include "cc-common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char prog_name[] = "xml2pg";

static char *find_tok(const char *pat, char *str) {
    const char *pat_ptr;
    char *str_ptr;
    int ch;

    while (*str) {
	str_ptr = str;
	pat_ptr = pat;
	while ((ch = *pat_ptr++) && ch == *str_ptr)
	    str_ptr++;
	if (!ch)
	    return str_ptr;
	str++;
    }
    return NULL;
}

static char *parse_int(const char *pat, size_t min, size_t max,
		       char *str, char **start) {
    char *str_start, *str_ptr;
    int ch;
    size_t len;

    if ((str_ptr = find_tok(pat, str))) {
	*start = str_start = str_ptr;
	do
	    ch = *str_ptr++;
	while (ch >= '0' && ch <= '9');
	if (ch == '<' || ch =='.') {
	    len = --str_ptr - str_start;
	    if (len >= min && len <= max) {
		*str_ptr = '\0';
		return ++str_ptr;
	    }
	}
    }
    return NULL;
}

static char *parse_real(const char *pat, char *str, char **start) {
    char *str_start, *str_ptr;
    int ch;

    if ((str_ptr = find_tok(pat, str))) {
	*start = str_start = str_ptr;
	do
	    ch = *str_ptr++;
	while ((ch >= '0' && ch <= '9') || ch == '.');
	if (ch == '<') {
	    if (--str_ptr > str_start) {
		*str_ptr = '\0';
		return ++str_ptr;
	    }
	}
    }
    return NULL;
}

static void line_out(FILE*out, time_t secs, unsigned usecs, const char *sensor,
		     const char *id, const char *tmpr, char *tail) {
    struct tm *tp;

    tp = gmtime(&secs);
    fprintf(out, "%04d-%02d-%02d %02d:%02d:%02d.%06u\t%s\t%s\t%s\t%s\n",
	    tp->tm_year + 1900, tp->tm_mon + 1, tp->tm_mday, tp->tm_hour,
	    tp->tm_min, tp->tm_sec, usecs, sensor, id, tmpr, tail);
}

static void xml2pg(FILE *power, FILE *pulse, FILE *in) {
    char line[MAX_LINE_LEN];
    char *ptr, *tstamp, *sensor, *tail, *tmpr, *id;
    time_t this_secs, last_secs;
    unsigned this_usecs, last_usecs;

    last_secs = last_usecs = 0;
    while (fgets(line, sizeof(line), in)) {
	if ((ptr = parse_int("<host-tstamp>", 10, 11, line, &tstamp))) {
	    this_secs = atoi(tstamp);
            this_usecs = strtol(ptr, &tail, 10);
	    if (tail > ptr) {
		last_secs = this_secs;
		last_usecs = this_usecs;
	    } else if (this_secs <= last_secs) {
	      this_secs = last_secs;
		this_usecs = ++last_usecs;
		putc('.', stderr);
	    } else {
		last_secs = this_secs;
		last_usecs = this_usecs = 0;
	    }
	    if ((ptr = parse_real("<tmpr>", ptr, &tmpr))) {
		if ((ptr = parse_int("<sensor>", 1, 1, ptr, &sensor))) {
		    if ((ptr = parse_int("<id>", 5, 5, ptr, &id))) {
			if (parse_real("<watts>", ptr, &tail))
			    line_out(power, this_secs, this_usecs, sensor, id, tmpr, tail);
			else if (parse_int("<imp>", 1, 12, ptr, &tail))
			    line_out(pulse, this_secs, this_usecs, sensor, id, tmpr, tail);
		    }
		}
	    }
	}
    }
}

int main(int argc, char **argv) {
    int  status;
    FILE *power, *pulse, *in;
    const char *file;

    if ((power = fopen("power.pg", "w"))) {
	if ((pulse = fopen("pulse.pg", "w"))) {
	    status = 0;
	    if (argc == 1)
		xml2pg(power, pulse, stdin);
	    else {
		while (--argc) {
		    file = *++argv;
		    if ((in = fopen(file, "r"))) {
		        fprintf(stderr, "%s\n", file);
			xml2pg(power, pulse, in);
			fclose(in);
		    } else {
		        log_syserr("unable to open '%s' for reading", file);
			status = 3;
		    }
		}
	    }
	    fclose(pulse);
	} else {
	    log_syserr("unable to open pulse file pulse.pg for writing");
	    status = 2;
	}
	fclose(power);
    } else {
	log_syserr("unable to open power file power.pg for writing");
	status = 2;
    }
    return status;
}
