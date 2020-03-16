#define _XOPEN_SOURCE
#include <time.h>

#include "cgi-main.h"
#include "cc-html.h"
#include "parsefile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char prog_name[] = "cc-now";

struct latest {
    time_t timestamp;
    double temp;
    double watts[MAX_SENSOR];
};

static mf_status filter_cb(pf_context *ctx, time_t ts)
{
    struct latest *l = ctx->user_data;
    if (ts < (l->timestamp - 120))
        return MF_STOP;
    if (l->timestamp <= 0)
        l->timestamp = ts;
    return MF_SUCCESS;
}

static mf_status sample_cb(pf_context *ctx, pf_sample * smp)
{
    struct latest *l = ctx->user_data;

    if (l->temp < 0)
        l->temp = smp->temp;
    if (smp->sensor >= 0 && smp->sensor < MAX_SENSOR)
        if (l->watts[smp->sensor] < 0)
            l->watts[smp->sensor] = smp->data.watts;
    return MF_SUCCESS;
}

/* *INDENT-OFF* */
static const char http_hdr[] =
    "Content-Type: text/html; charset=utf-8\n"
    "Refresh: 3\n";

static const char html_middle[] =
    "    <meta name=\"viewport\" content=\"initial-scale=2\"/>\n"
    "    <title>Energy Use Now</title>\n"
    "  </head>\n"
    "  <body>\n"
    "    <p><a href=\"%scc-picker.cgi?sens=%x\">Browse Consumption History</a></p>\n"
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
    "    <p>%s</p>\n"
    "    <p><a href=\"%scc-picker.cgi?sens=%x\">Browse Consumption History</a></p>\n";
/* *INDENT-ON* */

static void output_cell(double value, const char *label, FILE *cgi_str)
{
    const char *fmt = "<tr><td>%s</td><td>%.3g watts</td></tr>\n";

    if (value >= 1000) {
        fmt = "<tr><td>%s</td><td>%.2f Kw</td></tr>\n";
        value /= 1000;
    }
    fprintf(cgi_str, fmt, label, value);
}

static void cgi_output(struct latest *l, unsigned sens, FILE *cgi_str)
{
    int i;
    double value, apps, total;
    struct tm *tp;
    char tmstr[30];

    fwrite(http_hdr, sizeof(http_hdr) - 1, 1, cgi_str);
    html_send_top(cgi_str);
    fprintf(cgi_str, html_middle, base_url, sens);
    apps = 0.0;
    for (i = 0; i < MAX_SENSOR; i++) {
        value = l->watts[i];
        if (i >= 1 && i <= 5)
            apps += value;
        if (value >= 0)
            output_cell(value, sensor_names[i], cgi_str);
    }
    if (l->watts[9] > 0)
        total = l->watts[8] + l->watts[9];
    else
        total = l->watts[8] - l->watts[0];
    if (total >= 0) {
        output_cell(total, "Total Consumption", cgi_str);
        value = total - apps;
        if (value >= 0)
            output_cell(value, "Others", cgi_str);
    }
    tp = localtime(&l->timestamp);
    strftime(tmstr, sizeof tmstr, "%d/%m/%Y&nbsp;%H:%M:%S", tp);
    fprintf(cgi_str, html_bottom, l->temp, tmstr, base_url, sens);
    html_send_tail(cgi_str);
}

int cgi_main(struct timespec *start, cgi_query_t *query, FILE *cgi_str)
{
    int status = 2;
    time_t secs;
    struct tm *tp;
    char name[30], *ptr;
    pf_context *pf;
    struct latest l;
    int i;
    unsigned sens;

    if (chdir(default_dir) == 0) {
        secs = start->tv_sec - 6;       /* may need a sample six seconds ago */
        tp = gmtime(&secs);
        strftime(name, sizeof(name), xml_file, tp);
        if ((pf = pf_new())) {
            pf->file_cb = tf_parse_cb_backward;
            pf->filter_cb = filter_cb;
            pf->sample_cb = sample_cb;
            pf->user_data = &l;
            l.timestamp = 0;
            l.temp = -1.0;
            for (i = 0; i < MAX_SENSOR; i++) {
                l.watts[i] = -1.0;
            }
            if (pf_parse_file(pf, name) != MF_FAIL) {
                sens = 0;
                if ((ptr = cgi_get_param(query, "sens")))
                    sens = strtoul(ptr, NULL, 16);
                cgi_output(&l, sens, cgi_str);
                status = 0;
            }
            pf_free(pf);
        }
    }
    else
        log_syserr("unable to chdir to '%s'", default_dir);
    return status;
}
