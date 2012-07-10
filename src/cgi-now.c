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
    char   timestamp_iso[ISO_DATE_LEN];
    time_t timestamp_secs;
    double temp;
    double sensors[MAX_SENSOR];
};

static time_t parse_ts(const char *ts) {
    int year, month, day, hour, min, sec;
    struct tm ts_tm;
    if (sscanf(ts, "%4d-%2d-%2dT%2d:%2d:%2d", &year, &month, &day, &hour, &min, &sec) == 6) {
	memset(&ts_tm, 0, sizeof ts_tm);
	ts_tm.tm_year = year - 1900;
	ts_tm.tm_mon = month - 1;
	ts_tm.tm_mday = day;
	ts_tm.tm_hour = hour;
	ts_tm.tm_min = min;
	ts_tm.tm_sec = sec;
	return mktime(&ts_tm);
    } else {
	log_msg("unparsable time stamp '%s'", ts);
	return (time_t)-1;
    }
}
	
static pf_status sample_cb(pf_context *ctx, pf_sample *smp) {
    pf_status status = PF_SUCCESS;
    struct latest *l = ctx->user_data;
    struct tm ts;
    time_t secs;

    memset(&ts, 0, sizeof ts);
    if ((secs = parse_ts(smp->timestamp)) != -1) {
	if (secs > l->timestamp_secs-30) {
	    if (l->timestamp_secs <= 0) {
		l->timestamp_secs = secs;
		strncpy(l->timestamp_iso, smp->timestamp, 16);
	    }
	    if (l->temp < 0)
		l->temp = smp->temp;
	    if (smp->sensor >= 0 && smp->sensor < MAX_SENSOR)
		if (l->sensors[smp->sensor] < 0)
		    l->sensors[smp->sensor] = smp->watts;
	} else
	    status = PF_STOP;
    } else
	log_msg("invalid timestamp '%s'", smp->timestamp);
    return status;
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
    tp = localtime(&l->timestamp_secs);
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
	    pf->sample_cb = sample_cb;
	    pf->user_data = &l;
	    l.timestamp_secs = 0;
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
	
	
