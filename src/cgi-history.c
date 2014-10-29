#include "cc-common.h"
#include "history.h"
#include "cgi-common.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SECS_IN_HOUR (60*60)
#define SECS_IN_DAY  (SECS_IN_HOUR*24)
#define SECS_IN_WEEK (SECS_IN_DAY*7)

const char prog_name[] = "cc-history";
const char time_fmt[] = "%d/%m/%Y %H:%M:%S";

/* *INDENT-OFF* */
static const char http_hdr[] =
    "Content-Type: text/html\n";

static const char html_middle[] =
    "    <meta name=\"viewport\" content=\"initial-scale=1\"/>\n"
    "    <title>Historic Engery Use %s to %s</title>\n"
    "    <script src=\"/currentcost/js-class.js\"></script>\n"
    "    <script src=\"/currentcost/bluff-src.js\"></script>\n"
    "  </head>\n"
    "  <body>\n";

static const char graph_head[] =
    "    <canvas id=\"graph\" width=\"1280\" height=\"720\"></canvas>\n"
    "    <script type=\"text/javascript\">\n"
    "var g = new Bluff.Line('graph', '1280x720');\n"
    "g.title_font_size = 16;\n"
    "g.legend_font_size = 10;\n"
    "g.title = '%s to %s';\n"
    "g.tooltips = true;\n"
    "g.theme_keynote();\n";

static const char graph_end[] =
    "};\n"
    "g.draw();\n"
    "    </script>\n";
/* *INDENT-ON* */

static void send_labels(time_t start, time_t end, time_t delta, time_t step)
{
    time_t label_step, label;
    const char *label_fmt;
    char tmstr[20];
    int ch;

    if (delta <= SECS_IN_HOUR * 5) {
        label_fmt = "%H:%M";
        if (delta <= SECS_IN_HOUR)
            label_step = 360;
        else
            label_step = 1800;
    } else if (delta <= SECS_IN_DAY) {
        label_step = SECS_IN_HOUR * 2;
        label_fmt = "%H";
    } else if (delta <= SECS_IN_WEEK) {
        label_step = SECS_IN_DAY;
        label_fmt = "%a";
    } else {
        label_step = 86400 * 4;
        label_fmt = "%d";
    }
    label = start;
    fputs("g.labels=", stdout);
    ch = '{';
    while (label <= end) {
        strftime(tmstr, sizeof tmstr, label_fmt, localtime(&label));
        printf("%c%ld:'%s'", ch, (long) ((label - start) / step), tmstr);
        ch = ',';
        label += label_step;
    }
}

static void send_hist_link(time_t start, time_t end, const char *desc)
{
    printf("<a href=\"%scc-history.cgi?start=%lu&end=%lu\">%s</a>&nbsp;\n",
           base_url, start, end, desc);
}

static void send_navlinks(time_t start, time_t end, time_t delta)
{
    time_t half = delta / 2;
    time_t qtr = delta / 4;

    fputs("    <p>\n", stdout);
    send_hist_link(start - delta, end - delta, "<<<");
    send_hist_link(start - half, end - half, "<<");
    send_hist_link(start - qtr, end - qtr, "<");
    printf("<a href=\"%scc-now.cgi\">Current Consumption</a>\n", base_url);
    printf("<a href=\"%scc-picker.cgi\">Browse History</a>\n", base_url);
    send_hist_link(start + qtr, end + qtr, ">");
    send_hist_link(start + half, end + half, ">>");
    send_hist_link(start + delta, end + delta, ">>>");
    fputs("    </p>\n", stdout);
}

static int cgi_history(time_t start, time_t end)
{
    int status, i;
    time_t delta, step;
    hist_context *hc;
    char tm_from[20], tm_to[20];

    delta = end - start;
    step = delta / 720;
    if (step == 0)
        step = 1;
    if (chdir(default_dir) == 0) {
        strftime(tm_from, sizeof(tm_from), time_fmt, localtime(&start));
        strftime(tm_to, sizeof(tm_to), time_fmt, localtime(&end));
        log_msg("from %s to %s", tm_from, tm_to);
        if ((hc = hist_get(start, end, step))) {
            status = 0;
            fwrite(http_hdr, sizeof(http_hdr) - 1, 1, stdout);
            send_html_top(stdout);
            printf(html_middle, tm_from, tm_to);
            send_navlinks(start, end, delta);
            printf(graph_head, tm_from, tm_to);
            for (i = 0; i < MAX_SENSOR; i++) {
                if (hc->flags[i]) {
                    printf("g.data(\"%s\", ", sensor_names[i]);
                    hist_js_sens_out(hc, stdout, i);
                    fputs(");\n", stdout);
                }
            }
            fputs("g.data(\"Total Consumption\", ", stdout);
            hist_js_total_out(hc, stdout);
            fputs(");\n", stdout);
            fputs("g.data(\"Others\", ", stdout);
            hist_js_others_out(hc, stdout);
            fputs(");\n", stdout);
            send_labels(start, end, delta, step);
            hist_free(hc);
            fwrite(graph_end, sizeof(graph_end) - 1, 1, stdout);
            send_navlinks(start, end, delta);
            send_html_tail(stdout);
        } else
            status = 3;
    } else {
        log_syserr("unable to chdir to '%s'", default_dir);
        status = 2;
    }
    return status;
}

static time_t parse_limit(const char *value, time_t now)
{
    long n;

    n = strtol(value, NULL, 10);
    if (*value == '+' || *value == '-')
        return now + n;
    return n;
}

int main(int argc, char **argv)
{
    int status = 2;
    cgi_query_t *q;
    const char *start_str, *end_str;
    time_t now, start_secs, end_secs;

    if ((q = cgi_get_query())) {
        status = 0;
        if ((start_str = cgi_get_param(q, "start")) == NULL) {
            log_msg("missing 'start' parameter");
            status = 1;
        }
        if ((end_str = cgi_get_param(q, "end")) == NULL) {
            log_msg("missing 'end' parameter");
            status = 1;
        }
        if (status == 0) {
            time(&now);
            start_secs = parse_limit(start_str, now);
            end_secs = parse_limit(end_str, now);
            if (end_secs <= start_secs) {
                log_msg("end must be greater than start");
                status = 1;
            } else
                status = cgi_history(start_secs, end_secs);
        }
    }
    return status;
}
