#define _XOPEN_SOURCE
#include <time.h>

#include "cc-common.h"
#include "parsefile.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

const char prog_name[] = "cc-now";

#define MAX_SENSOR 10

struct latest {
    time_t timestamp;
    double temp;
    double sensors[MAX_SENSOR];
};

static pf_status filter_cb(pf_context *ctx, time_t ts) {
    struct latest *l = ctx->user_data;
    if (ts < (l->timestamp - 30))
	return PF_STOP;
    if (l->timestamp <= 0)
	l->timestamp = ts;
    return PF_SUCCESS;
}

static pf_status sample_cb(pf_context *ctx, pf_sample *smp) {
    struct latest *l = ctx->user_data;

    if (l->temp < 0)
	l->temp = smp->temp;
    if (smp->sensor >= 0 && smp->sensor < MAX_SENSOR)
	if (l->sensors[smp->sensor] < 0)
	    l->sensors[smp->sensor] = smp->watts;
    return PF_SUCCESS;
}

static const char *sensor_names[] = {
    "Whole House",
    "Computers",
    "TV & Audio",
    "Sensor 3",
    "Sensor 4",
    "Sensor 5",
    "Sensor 6",
    "Sensor 7",
    "Sensor 8",
    "Sensor 9",
    "Sensor 10"
};

static const char html_top[] =
    "Content-Type: text/html\n"
    "Refresh: 3\n"
    "\n"
    "<html>\n"
    "  <head>\n"
    "    <title>Energy Use Now</title>\n"
    "    <meta name=\"HandheldFriendly\" content=\"true\"/>\n"
    "    <meta name=\"viewport\" content=\"target-densitydpi=device-dpi\"/>\n"
    "    <meta name=\"viewport\" content=\"initial-scale=2\"/>\n"
    "  </head>\n"
    "  <body>\n"
    "    <h1>Energy Use Now</h1>\n"
    "    <table>\n";

static const char html_tail[] =
    "    </table>\n"
    "  </body>\n"
    "</html>\n";

static void cgi_output(struct latest *l) {
    int i;
    double value;
    struct tm *tp;
    char tmstr[30];

    fwrite(html_top, (sizeof html_top)-1, 1, stdout);
    for (i = 0; i < MAX_SENSOR; i++) {
	value = l->sensors[i];
	if (value >= 0)
	    printf("<tr><td>%s</td><td>%g</td></tr>\n", sensor_names[i], value);
    }
    tp = localtime(&l->timestamp);
    strftime(tmstr, sizeof tmstr, "%d/%m/%Y %H:%M:%S", tp);
    printf("<tr><td>Time</td><td>%s</td></tr><tr><td>Temperature</td><td>%g</td>\n", tmstr, l->temp);
    fwrite(html_tail, (sizeof html_tail)-1, 1, stdout);
}

int main(int argc, char **argv) {
    int status = 1;
    time_t secs;
    struct tm *tp;
    char name[30];
    pf_context *pf;
    struct latest l;
    int i;

    if (chdir(default_dir) == 0) {
	time(&secs);
	secs -= 6; /* may need a sample six seconds ago */
	tp = gmtime(&secs);
	strftime(name, sizeof(name), xml_file, tp);
	if ((pf = pf_new())) {
	    pf->file_cb = pf_parse_backward;
	    pf->filter_cb = filter_cb;
	    pf->sample_cb = sample_cb;
	    pf->user_data = &l;
	    l.timestamp = 0;
	    l.temp = -1.0;
	    for (i = 0; i < MAX_SENSOR; i++)
		l.sensors[i] = -1.0;
	    if (pf_parse_file(pf, name) != PF_FAIL) {
		cgi_output(&l);
		status = 0;
	    }
	    pf_free(pf);
	}
    } else
	log_syserr("unable to chdir to '%s'", default_dir);
    return status;
}
	
	
