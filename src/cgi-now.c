#define _XOPEN_SOURCE
#include <time.h>

#include "cc-common.h"
#include "cc-html.h"
#include "cgi-dbmain.h"
#include "log-db-err.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

const char prog_name[] = "cc-now";

/* *INDENT-OFF* */
static const char http_hdr[] =
    "Content-Type: text/html\n"
    "Refresh: 3\n";

static const char html_middle[] =
    "    <meta name=\"viewport\" content=\"initial-scale=2\"/>\n"
    "    <title>Energy Use Now</title>\n"
    "  </head>\n"
    "  <body>\n"
    "    <p><a href=\"%scc-picker.cgi\">Browse Consumption History</a></p>\n"
    "    <h1>Energy Use Now</h1>\n"
    "    <table>\n"
    "      <thead>\n"
    "        <tr>\n"
    "          <th>Sensor</th>\n"
    "          <th>Consumption</th>\n"
    "          <th>Temperature</th>\n"
    "        </tr>\n"
    "      </thead>\n"
    "      <tbody>\n";

static const char html_bottom[] =
    "      </tbody>\n"
    "    </table>\n"
    "    <p>%s</p>\n"
    "    <p><a href=\"%scc-picker.cgi\">Browse Consumption History</a></p>\n";
/* *INDENT-ON* */

static const char sql[] =
    "SELECT     s.label,p.watts,p.temperature "
    "FROM       (SELECT sensor,MAX(time_stamp) AS mts "
    "            FROM   power "
    "            WHERE  time_stamp > (current_timestamp-interval '30s') "
    "            GROUP BY sensor) t "
    "INNER JOIN power p   ON p.time_stamp=t.mts AND p.sensor=t.sensor "
    "INNER JOIN sensors s ON s.ix = p.sensor "
    "ORDER BY s.ix";

static void html_result(PGresult *res) {
    int rows, r, cols, c;

    rows = PQntuples(res);
    cols = PQnfields(res);
    for (r = 0; r < rows; r++) {
	cgi_out_str("<tr>\n");
	for (c = 0; c < cols; c++) {
	    cgi_out_str("<td>");
	    cgi_out_htmlesc(PQgetvalue(res, r, c));
	    cgi_out_str("</td>");
	}
	cgi_out_str("</tr>\n");
    }
}

int cgi_db_main(struct timespec *start, cgi_query_t *query, PGconn *conn) {
    int       status;
    PGresult  *res;
    time_t    now;
    struct tm *tp;
    char      tmstr[25];

    if ((res = PQexec(conn, sql))) {
	if (PQresultStatus(res) == PGRES_TUPLES_OK) {
	    cgi_out_text(http_hdr, sizeof(http_hdr)-1);
	    send_html_top();
	    cgi_out_printf(html_middle, base_url);
	    html_result(res);
	    PQclear(res);
	    time(&now);
	    tp = localtime(&now);
	    strftime(tmstr, sizeof tmstr, "%d/%m/%Y&nbsp;%H:%M:%S", tp);
	    cgi_out_printf(html_bottom, tmstr, base_url);
	    send_html_tail();
	    status = 0;
	} else {
	    log_db_err(conn, "query failed");
	    status = 4;
	}
    } else {
	log_syserr("unable to allocate query result");
	status = 4;
    }
    return status;
}
