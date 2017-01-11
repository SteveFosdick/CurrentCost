#include "cc-html.h"
#include "cc-rusage.h"
#include "history.h"
#include "cgi-main.h"

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

static const char form_head[] =
    "    <form method=\"POST\">\n"
    "      <input type=\"hidden\" name=\"start\" value=\"%ld\">\n"
    "      <input type=\"hidden\" name=\"end\" value=\"%ld\">\n";

static const char form_tail[] =
    "     <input type=\"submit\" name=\"go\" value=\"Filter\">"
    "   </form>\n";

/* *INDENT-ON* */

static void send_labels(time_t start, time_t end, time_t delta, time_t step, FILE *cgi_str)
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
    html_puts("g.labels=", cgi_str);
    ch = '{';
    while (label <= end) {
        strftime(tmstr, sizeof tmstr, label_fmt, localtime(&label));
        fprintf(cgi_str, "%c%ld:'%s'", ch, (long) ((label - start) / step), tmstr);
        ch = ',';
        label += label_step;
    }
}

static void send_hist_link(time_t start, time_t end, const char *desc,
			   unsigned sens, FILE *cgi_str)
{
    fprintf(cgi_str, "<a href=\"%scc-history.cgi?start=%lu&end=%lu&sens=%x\">%s</a>&nbsp;\n",
	    base_url, start, end, sens, desc);
}

static void send_navlinks(time_t start, time_t end, time_t delta, unsigned sens, FILE *cgi_str)
{
    time_t half = delta / 2;
    time_t qtr = delta / 4;

    html_puts("    <p>\n", cgi_str);
    send_hist_link(start - delta, end - delta, "<<", sens, cgi_str);
    send_hist_link(start - half, end - half, "<", sens, cgi_str);
    send_hist_link(start + qtr, end - qtr, "+", sens, cgi_str);
    send_hist_link(start - qtr, end + qtr, "-", sens, cgi_str);
    send_hist_link(start + half, end + half, ">", sens, cgi_str);
    send_hist_link(start + delta, end + delta, ">>", sens, cgi_str);
    fprintf(cgi_str, "<a href=\"%scc-now.cgi?sens=%x\">Current Consumption</a>\n", base_url, sens);
    fprintf(cgi_str, "<a href=\"%scc-picker.cgi?sens=%x\">Browse History</a>\n", base_url, sens);
    html_puts("    </p>\n", cgi_str);
}

static void send_checkboxes(time_t start, time_t end, unsigned sens, FILE *cgi_str) {
    int i;
    const char *chk;
    
    fprintf(cgi_str, form_head, start, end);
    for (i = 0; i < MAX_SENSOR; i++) {
	chk = sens & (1 << i) ? "" : " checked";
	fprintf(cgi_str, "<input type=\"checkbox\" name=\"s%d\" value=\"on\"%s>&nbsp;%s\n",
		i, chk, sensor_names[i]);
    }
    fwrite(form_tail, sizeof(form_tail)-1, 1, cgi_str);
}

static int cgi_history(struct timespec *prog_start, time_t start, time_t end,
		       unsigned sens, FILE *cgi_str)
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
            fwrite(http_hdr, sizeof(http_hdr)-1, 1, cgi_str);
            html_send_top(cgi_str);
            fprintf(cgi_str, html_middle, tm_from, tm_to);
            send_navlinks(start, end, delta, sens, cgi_str);
            fprintf(cgi_str, graph_head, tm_from, tm_to);
            for (i = 0; i < MAX_SENSOR; i++) {
		if (!(sens & (1 << i))) {
		    if (hc->flags[i]) {
			fprintf(cgi_str, "g.data(\"%s\", ", sensor_names[i]);
			hist_js_sens_out(hc, i, cgi_str);
			html_puts(");\n", cgi_str);
		    }
		}
            }
            html_puts("g.data(\"Total Consumption\", ", cgi_str);
            hist_js_total_out(hc, cgi_str);
            html_puts(");\n", cgi_str);
            html_puts("g.data(\"Others\", ", cgi_str);
            hist_js_others_out(hc, cgi_str);
            html_puts(");\n", cgi_str);
            send_labels(start, end, delta, step, cgi_str);
            hist_free(hc);
            fwrite(graph_end, sizeof(graph_end)-1, 1, cgi_str);
            send_navlinks(start, end, delta, sens, cgi_str);
	    send_checkboxes(start, end, sens, cgi_str);
	    cc_rusage(prog_start, cgi_str);
            html_send_tail(cgi_str);
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

static int which_sensors(cgi_query_t *query) {
    int i;
    unsigned bits = 0;
    char name[3], *ptr;

    if ((ptr = cgi_get_param(query, "sens")))
	bits = strtoul(ptr, NULL, 16);
    else {
	strcpy(name, "sx");
	for (i = 0; i < MAX_SENSOR; i++) {
	    name[1] = '0' + i;
	    if ((ptr = cgi_get_param(query, name)) == NULL || strcmp(ptr, "on"))
		bits |= (1 << i);
	}
    }
    return bits;
}

int cgi_main(struct timespec *start, cgi_query_t *query, FILE *cgi_str) {
    int status = 0;
    const char *start_str, *end_str;
    time_t start_secs, end_secs;

    if ((start_str = cgi_get_param(query, "start")) == NULL) {
	log_msg("missing 'start' parameter");
	status = 1;
    }
    if ((end_str = cgi_get_param(query, "end")) == NULL) {
	log_msg("missing 'end' parameter");
	status = 1;
    }
    if (status == 0) {
	start_secs = parse_limit(start_str, start->tv_sec);
	end_secs = parse_limit(end_str, start->tv_sec);
	if (end_secs <= start_secs) {
	    log_msg("end must be greater than start");
	    status = 1;
	} else
	    status = cgi_history(start, start_secs, end_secs, which_sensors(query), cgi_str);
    }
    return status;
}
