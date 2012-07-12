#define _XOPEN_SOURCE
#include <time.h>

#include "cc-common.h"
#include "parsefile.h"
#include "cgi-common.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

const char prog_name[] = "cc-now";

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

static const char http_hdr[] =
    "Content-Type: text/html\n"
    "Refresh: 3\n";

static const char html_middle[] =
    "    <title>Energy Use Now</title>\n"
    "  </head>\n"
    "  <body>\n"
    "    <h1>Energy Use Now</h1>\n"
    "    <table>\n"
    "      <thead>\n"
    "        <tr>\n"
    "          <th>Sensor</th>\n"
    "          <th>Consumption</th>\n"
    "        </tr>\n"
    "      </thead>\n"
    "      <tbody>\n";

static const char html_bottom[] =
    "        <tr>\n"
    "          <td>Temperature</td>\n"
    "          <td>%g</td>\n"
    "        </tr>\n"
    "      </tbody>\n"
    "    </table>\n"
    "    <p>%s</p>\n";

static void cgi_output(struct latest *l) {
    int i;
    double value;
    struct tm *tp;
    char tmstr[30];

    fwrite(http_hdr, sizeof(http_hdr)-1, 1, stdout);
    send_html_top(stdout);
    fwrite(html_middle, (sizeof html_middle)-1, 1, stdout);
    for (i = 0; i < MAX_SENSOR; i++) {
	value = l->sensors[i];
	if (value >= 0) {
	    printf("<tr><td>%s</td><td>", sensor_names[i]);
	    if (value >= 1000)
		printf("%.2f Kw</td></tr>\n", value / 1000);
	    else
		printf("%g watts</td></tr>\n", value);
	}
    }
    tp = localtime(&l->timestamp);
    strftime(tmstr, sizeof tmstr, "%d/%m/%Y&nbsp;%H:%M:%S", tp);
    printf(html_bottom, l->temp, tmstr);
    send_html_tail(stdout);
}

int main(int argc, char **argv) {
    int status = 2;
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
    } else {
	log_syserr("unable to chdir to '%s'", default_dir);
	status = 2;
    }
    return status;
}
	
	
