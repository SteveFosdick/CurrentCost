#include "cc-common.h"
#include "history.h"
#include "cgi-common.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char prog_name[] = "cc-history";

static const char http_hdr[] =
    "Content-Type: text/html\n";

static const char html_middle[] =
    "    <title>Historic Engery Use - %s</title>\n"
    "    <script src=\"/currentcost/js-class.js\"></script>\n"
    "    <script src=\"/currentcost/bluff-src.js\"></script>\n"
    "  </head>\n"
    "  <body>\n"
    "    <canvas id=\"graph\" width=\"800\" height=\"600\"></canvas>\n"
    "    <script type=\"text/javascript\">\n"
    "var g = new Bluff.Line('graph', '800x600');\n"
    "g.title = 'Historic Engery Use - %s';\n"
    "g.tooltips = true;\n"
    "g.theme_keynote();\n";

static const char html_bottom[] =
    "};\n"
    "g.draw();\n"
    "    </script>\n";

int cgi_main(int argc, char **argv) {
    int status;
    time_t start, end, delta, step, label_step, label;
    const char *graph_title, *label_fmt, *query;
    hist_context *hc;
    int i, ch;
    char tmstr[10];

    delta = 3600; /* hour */
    label_step = 600;
    label_fmt = "%H:%M";
    graph_title = "Last Hour";
    if ((query = getenv("QUERY_STRING"))) {
	if (strstr(query, "day")) {
	    delta = 86400;
	    label_step = 3600 * 4;
	    label_fmt = "%H";
	    graph_title = "Last 24hr";
	} else if (strstr(query, "week")) {
	    delta = 86400 * 7;
	    label_step = 86400;
	    label_fmt = "%a";
	    graph_title = "Last Week";
	} else if (strstr(query, "month")) {
	    delta = 86400 * 30;
	    label_step = 86400 * 4;
	    label_fmt = "%d";
	    graph_title = "Last Month";
	}
    }
    step = delta / 720;
    if (step == 0)
	step = 1;
    time(&end);
    start = end - delta;
    if ((hc = hist_get(start, end, step))) {
	status = 0;
	fwrite(http_hdr, sizeof(http_hdr)-1, 1, stdout);
	send_html_top(stdout);
	printf(html_middle, graph_title, graph_title);
	for (i = 0; i < MAX_SENSOR; i++) {
	    if (hc->flags[i]) {
		printf("g.data(\"%s\", ", sensor_names[i]);
		hist_js_sens_out(hc, stdout, i);
		fputs(");\n", stdout);
	    }
	}
	hist_free(hc);
	label = start + label_step - (start % label_step);
	fputs("g.labels=", stdout);
	ch = '{';
	while (label < end) {
	    strftime(tmstr, sizeof tmstr, label_fmt, localtime(&label));
	    printf("%c%ld:'%s'", ch, (long)((label-start)/step), tmstr);
	    ch = ',';
	    label += label_step;
	}
	fwrite(html_bottom, sizeof(html_bottom)-1, 1, stdout);
	send_html_tail(stdout);
    } else
	status = 2;
    return status;
}
