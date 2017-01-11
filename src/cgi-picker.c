#include "cgi-main.h"
#include "cc-html.h"

#include <stdlib.h>
#include <time.h>

const char prog_name[] = "cc-picker";

/* *INDENT-OFF* */
static const char http_hdr[] =
    "Content-Type: text/html\n";

static const char html_middle[] =
    "    <meta name=\"viewport\" content=\"initial-scale=2\"/>\n"
    "    <title>Current Cost Date Picker</title>\n"
    "  </head>\n"
    "  <body>\n"
    "    <p><a href=\"%scc-now.cgi\">Current Consumption</a></p>\n"
    "    <h1>Browse History</h1>\n"
    "    <table>\n";

static const char cal_top[] =
    "      <caption>%s</caption>\n"
    "      <thead>\n"
    "        <tr>\n"
    "          <th>Mon</th>\n"
    "          <th>Tue</th>\n"
    "          <th>Wed</th>\n"
    "          <th>Thu</th>\n"
    "          <th>Fri</th>\n"
    "          <th>Sat</th>\n"
    "          <th>Sun</th>\n"
    "        </tr>\n"
    "      </thead>\n"
    "      <tbody>\n";

static const char tab_top[] =
    "    <table>\n"
    "      <tbody>\n";

static const char tab_tail[] =
    "      </tbody>\n"
    "    </table>\n";

static const char new_para[] =
    "    </p>\n"
    "    <p>\n";

static const char tab_end[] =
    "      </tbody>\n"
    "    </table>\n"
    "    <p><a href=\"%scc-now.cgi\">Current Consumption</a></p>\n";
/* *INDENT-ON* */

static void send_calendar(time_t start_secs, struct tm *tp, unsigned sens, FILE *cgi_str) {
    time_t secs;
    int home_month, i;
    char caption[50];

    secs = start_secs;
    strftime(caption, sizeof caption, "%B %Y", tp);
    fprintf(cgi_str, cal_top, caption);
    home_month = tp->tm_mon;
    tp->tm_mday = 1;
    secs = mktime(tp);
    while (tp->tm_wday != 1) {
        secs -= 86400;
        tp = localtime(&secs);
    }
    do {
        html_puts("<tr>\n", cgi_str);
        for (i = 0; i < 7; i++) {
            if (secs == start_secs)
                fprintf(cgi_str, "<td>%d</td>\n", tp->tm_mday);
            else
                fprintf(cgi_str,
			"<td><a href=\"%scc-picker.cgi?start=%lu&sens=%x\">%2d</a></td>\n",
			base_url, secs, sens, tp->tm_mday);
            secs += 86400;
            tp = localtime(&secs);
        }
        html_puts("</tr>\n", cgi_str);
    } while (tp->tm_mon == home_month);
    fwrite(tab_tail, sizeof(tab_tail)-1, 1, cgi_str);
}

static void send_hist_link(time_t start, time_t end, const char *desc, unsigned sens, FILE *cgi_str) {
    fprintf(cgi_str, "<a href=\"%scc-history.cgi?start=%lu&end=%lu&sens=%x\">%s</a>\n",
	    base_url, start, end, sens, desc);
}

static void send_middle_links(time_t midnight, struct tm *start_tp, unsigned sens, FILE *cgi_str) {
    time_t start, end;
    struct tm *tp;

    html_puts("    <p>\n", cgi_str);
    start = midnight - 3600;
    end = midnight + 8 * 3600;
    send_hist_link(start, end, "Night", sens, cgi_str);
    start = end;
    end = midnight + 13 * 3600;
    send_hist_link(start, end, "Morning", sens, cgi_str);
    start = end;
    end = midnight + 18 * 3600;
    send_hist_link(start, end, "Afternoon", sens, cgi_str);
    start = end;
    end = midnight + 23 * 3600;
    send_hist_link(start, end, "Evening", sens, cgi_str);

    fwrite(new_para, sizeof(new_para)-1, 1, cgi_str);

    end = midnight + 86400;
    send_hist_link(midnight, end, "Whole Day", sens, cgi_str);

    start = midnight;
    tp = start_tp;
    while (tp->tm_wday != 1) {
        start -= 86400;
        tp = localtime(&start);
    }
    end = start + (86400 * 7);
    send_hist_link(start, end, "Whole Week", sens, cgi_str);

    start = midnight;
    tp = start_tp;
    while (tp->tm_mday != 1) {
        start -= 86400;
        tp = localtime(&start);
    }
    end = midnight;
    do {
        end += 86400;
        tp = localtime(&end);
    } while (tp->tm_mday != 1);
    end -= 86400;
    send_hist_link(start, end, "Whole Month", sens, cgi_str);
    html_puts("    </p>\n", cgi_str);
}

static void send_hour_links(time_t secs, unsigned sens, FILE *cgi_str)
{
    int i, j, hour, flag;
    const char *mins = "00";

    fwrite(tab_top, sizeof(tab_top)-1, 1, cgi_str);
    hour = flag = 0;
    for (i = 0; i < 8; i++) {
        html_puts("<tr>\n", cgi_str);
        for (j = 0; j < 6; j++) {
            fprintf(cgi_str,
		    "<td><a href=\"%scc-history.cgi?start=%lu&end=%lu&sens=%x\">%02d:%s</a></td>\n",
		    base_url, secs, secs + 3600, sens, hour, mins);
            secs += 1800;
            if (flag) {
                mins = "00";
                hour++;
                flag = 0;
            } else {
                mins = "30";
                flag = 1;
            }
        }
        html_puts("</tr>\n", cgi_str);
    }
}

int cgi_main(struct timespec *start, cgi_query_t *query, FILE *cgi_str) {
    const char *start_str, *ptr;
    time_t start_secs, midnight;
    struct tm start_tm;
    char tmstr[20];
    unsigned sens;

    if ((start_str = cgi_get_param(query, "start")))
	start_secs = strtoul(start_str, NULL, 10);
    else
	time(&start_secs);
    localtime_r(&start_secs, &start_tm);
    start_tm.tm_hour = 0;   /* midnight */
    start_tm.tm_min = 0;
    start_tm.tm_sec = 0;
    midnight = mktime(&start_tm);
    strftime(tmstr, sizeof tmstr, date_iso, localtime(&midnight));
    log_msg("midnight is %s", tmstr);

    sens = 0;
    if ((ptr = cgi_get_param(query, "sens")))
	sens = strtoul(ptr, NULL, 16);

    fwrite(http_hdr, sizeof(http_hdr)-1, 1, cgi_str);
    html_send_top(cgi_str);
    fprintf(cgi_str, html_middle, base_url);
    send_calendar(midnight, &start_tm, sens, cgi_str);
    send_middle_links(midnight, &start_tm, sens, cgi_str);
    send_hour_links(midnight, sens, cgi_str);
    fprintf(cgi_str, tab_end, base_url);
    html_send_tail(cgi_str);
    return 0;
}
