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

static char *mark_end(char *ptr) {
    int ch;

    do
	ch = *ptr++;
    while ((ch >= '0' && ch <= '9') || ch == '.');
    if (ch == '<') {
	ptr[-1] = '\0';
	return ptr;
    }
    return NULL;
}

static void line_out(FILE*out, time_t secs, unsigned usecs,
		     const char *id, const char *tmpr, char *tail) {
    struct tm *tp;

    if (mark_end(tail)) {
	tp = gmtime(&secs);
	fprintf(out, "%04d-%02d-%02d %02d:%02d:%02d.%06u+00\t%s\t%s\t%s\n",
		tp->tm_year + 1900, tp->tm_mon + 1, tp->tm_mday, tp->tm_hour,
		tp->tm_min, tp->tm_sec, usecs, id, tmpr, tail);
    }
}

static void xml2pg(FILE *power, FILE *pulse, FILE *in) {
    char line[MAX_LINE_LEN];
    char *ptr, *tail, *tmpr, *id;
    time_t this_secs, last_secs;
    unsigned this_usecs, last_usecs;

    last_secs = last_usecs = 0;
    while (fgets(line, sizeof(line), in)) {
	if ((ptr = find_tok("<host-tstamp>", line))) {
	    if ((tail = mark_end(ptr))) {
		this_secs = atoi(ptr);
		if (this_secs == last_secs)
		    this_usecs = ++last_usecs;
		else {
		    last_secs = this_secs;
		    last_usecs = this_usecs = 0;
		}
		if ((ptr = find_tok("<tmpr>", tail))) {
		    tmpr = ptr;
		    if ((ptr = mark_end(ptr))) {
			if ((ptr = find_tok("<id>", ptr))) {
			    id = ptr;
			    if ((ptr = mark_end(ptr))) {
				if ((tail = find_tok("<watts>", ptr)))
				    line_out(power, this_secs, this_usecs, id, tmpr, tail);
				else if ((tail = find_tok("<imp>", ptr)))
				    line_out(pulse, this_secs, this_usecs, id, tmpr, tail);
			    }
			}
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
			xml2pg(power, pulse, in);
			fclose(in);
		    } else {
			log_syserr("unable to open '%s' for reading", file);
			status = 3;
		    }
		}
	    }
	} else {
	    log_syserr("unable to open pulse file pulse.pg for writing");
	    status = 2;
	}
    } else {
	log_syserr("unable to open power file power.pg for writing");
	status = 2;
    }
    return status;
}
