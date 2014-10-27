#include "cc-common.h"
#include "cgi-common.h"

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

static void send_calendar(time_t start_secs, struct tm *tp)
{
    time_t secs;
    int home_month, i;
    char caption[50];

    secs = start_secs;
    strftime(caption, sizeof caption, "%B %Y", tp);
    printf(cal_top, caption);
    home_month = tp->tm_mon;
    tp->tm_mday = 1;
    secs = mktime(tp);
    while (tp->tm_wday != 1) {
        secs -= 86400;
        tp = localtime(&secs);
    }
    do {
        fputs("<tr>\n", stdout);
        for (i = 0; i < 7; i++) {
            if (secs == start_secs)
                printf("<td>%d</td>\n", tp->tm_mday);
            else
                printf
                    ("<td><a href=\"%scc-picker.cgi?start=%lu\">%2d</a></td>\n",
                     base_url, secs, tp->tm_mday);
            secs += 86400;
            tp = localtime(&secs);
        }
        fputs("</tr>\n", stdout);
    } while (tp->tm_mon == home_month);
    fwrite(tab_tail, sizeof(tab_tail) - 1, 1, stdout);
}

static void send_hist_link(time_t start, time_t end, const char *desc)
{
    printf("<a href=\"%scc-history.cgi?start=%lu&end=%lu\">%s</a>\n",
           base_url, start, end, desc);
}

static void send_middle_links(time_t midnight, struct tm *start_tp)
{
    time_t start, end;
    struct tm *tp;

    fputs("    <p>\n", stdout);
    start = midnight - 3600;
    end = midnight + 8 * 3600;
    send_hist_link(start, end, "Night");
    start = end;
    end = midnight + 13 * 3600;
    send_hist_link(start, end, "Morning");
    start = end;
    end = midnight + 18 * 3600;
    send_hist_link(start, end, "Afternoon");
    start = end;
    end = midnight + 23 * 3600;
    send_hist_link(start, end, "Evening");

    fwrite(new_para, sizeof(new_para) - 1, 1, stdout);

    end = midnight + 86400;
    send_hist_link(midnight, end, "Whole Day");

    start = midnight;
    tp = start_tp;
    while (tp->tm_wday != 1) {
        start -= 86400;
        tp = localtime(&start);
    }
    end = start + (86400 * 7);
    send_hist_link(start, end, "Whole Week");

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
    send_hist_link(start, end, "Whole Month");
    fputs("    </p>\n", stdout);
}

static void send_hour_links(time_t secs)
{
    int i, j, hour, flag;
    const char *mins = "00";

    fwrite(tab_top, sizeof(tab_top) - 1, 1, stdout);
    hour = flag = 0;
    for (i = 0; i < 8; i++) {
        fputs("<tr>\n", stdout);
        for (j = 0; j < 6; j++) {
            printf
                ("<td><a href=\"%scc-history.cgi?start=%lu&end=%lu\">%02d:%s</a></td>\n",
                 base_url, secs, secs + 3600, hour, mins);
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
        fputs("</tr>\n", stdout);
    }
}

int main(int argc, char **argv)
{
    int status = 1;
    cgi_query_t *q;
    const char *start_str;
    time_t start_secs, midnight;
    struct tm start_tm;
    char tmstr[20];

    if ((q = cgi_get_query())) {
        status = 0;
        if ((start_str = cgi_get_param(q, "start")))
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

        fwrite(http_hdr, sizeof(http_hdr) - 1, 1, stdout);
        send_html_top(stdout);
        printf(html_middle, base_url);
        send_calendar(midnight, &start_tm);
        send_middle_links(midnight, &start_tm);
        send_hour_links(midnight);
        printf(tab_end, base_url);
        send_html_tail(stdout);
    }
    return status;
}
