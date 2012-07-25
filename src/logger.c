#include "cc-common.h"
#include "logger.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum {
    ST_GROUND,
    ST_GOT_LT,
    ST_GOT_M,
    ST_GOT_S,
    ST_GOT_G,
    ST_COPY,
    ST_NON_ASCII
} state_t;

struct _logger_t {
    state_t state;
    time_t  switch_secs;
    FILE    *xml_fp;
};

logger_t *logger_new(void) {
    logger_t *logger;

    if ((logger = malloc(sizeof(logger_t)))) {
	logger->state = ST_GROUND;
	logger->switch_secs = 0;
	logger->xml_fp = NULL;
	return logger;
    }
    return NULL;
}

void logger_free(logger_t *logger) {
    if (logger->xml_fp)
	fclose(logger->xml_fp);
    free(logger);
}

static void switch_file(logger_t *logger, time_t now_secs) {
    struct tm *tp;
    FILE *nfp;
    char file[30];

    tp = gmtime(&now_secs);
    strftime(file, sizeof file, xml_file, tp);
    if ((nfp = fopen(file, "a"))) {
	if (logger->xml_fp != NULL)
	    fclose(logger->xml_fp);
	logger->xml_fp = nfp;
	logger->switch_secs = now_secs + 86400 - (now_secs % 86400);
    } else
	log_msg("unable to open file '%s' for append - %s",
		file, strerror(errno));
}

static void msg_begin(logger_t *logger) {
    time_t now_secs;

    time(&now_secs);
    if (now_secs >= logger->switch_secs)
	switch_file(logger, now_secs);
    if (logger->xml_fp != NULL)
	fprintf(logger->xml_fp, "<msg><host-tstamp>%lu</host-tstamp>",
		(unsigned long)now_secs);
}

static void msg_data(logger_t *logger,
		     const unsigned char *data, const unsigned char *end) {

    if (end > data)
	if (logger->xml_fp != NULL)
	    fwrite(data, end-data, 1, logger->xml_fp);
}

static void msg_end(logger_t *logger) {
    if (logger->xml_fp != NULL) {
	putc('\n', logger->xml_fp);
	fflush(logger->xml_fp);
    }
}

extern void logger_data(logger_t *logger,
			const unsigned char *data, size_t size) {

    const unsigned char *ptr = data;
    const unsigned char *end = data + size;
    const unsigned char *copy = NULL;
    state_t state = logger->state;
    int ch;

    if (state == ST_COPY)
	copy = data;
    while (ptr < end) {
	ch = *ptr++;
	switch(state) {
	case ST_GROUND:
	    if (ch == '<')
		state = ST_GOT_LT;
	    break;
	case ST_GOT_LT:
	    if (ch == 'm')
		state = ST_GOT_M;
	    else
		state = ST_GROUND;
	    break;
	case ST_GOT_M:
	    if (ch == 's')
		state = ST_GOT_S;
	    else
		state = ST_GROUND;
	    break;
	case ST_GOT_S:
	    if (ch == 'g')
		state = ST_GOT_G;
	    else
		state = ST_GROUND;
	    break;
	case ST_GOT_G:
	    if (ch == '>') {
		msg_begin(logger);
		state = ST_COPY;
		copy = ptr;
	    } else
		state = ST_GROUND;
	    break;
	case ST_COPY:
	    if (ch < 0x20 || ch > 0x7e) {
		msg_data(logger, copy, ptr-1);
		copy = NULL;
		if (ch == '\n') {
		    msg_end(logger);
		    state = ST_GROUND;
		}
		else
		    state = ST_NON_ASCII;
	    }
	    break;
	case ST_NON_ASCII:
	    if (ch == '\n') {
		msg_end(logger);
		state = ST_GROUND;
	    } else if (ch >= 0x20 || ch <= 0x7e) {
		copy = ptr;
		state = ST_COPY;
	    }
	}
    }
    if (copy)
	msg_data(logger, copy, ptr);
    logger->state = state;
}
