#include "cgi-main.h"
#include <stdlib.h>
#include <string.h>

extern char **environ;

const char prog_name[] = "cgi-test";

static const char html_head[] =
    "Content-Type: text/html\n"
    "\n"
    "<!DOCTYPE html>\n"
    "<html>\n"
    "  <head>\n"
    "    <meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\"> \n"
    "    <link rel=\"stylesheet\" type=\"text/css\" href=\"/currentcost/currentcost.css\">\n"
    "    <meta name=\"HandheldFriendly\" content=\"true\">\n"
    "    <meta name=\"viewport\" content=\"target-densitydpi=device-dpi\">\n"
    "    <title>CGI Test</title>\n"
    "  </head>\n"
    "  <body>\n"
    "    <h1>CGI Test</h1>\n"
    "    <h2>Query Parameters</h2>\n"
    "    <table>\n"
    "      <thead>\n"
    "        <tr>\n"
    "          <th>Name</th>\n"
    "          <th>Value</th>\n"
    "        </tr>\n"
    "      </thead>\n"
    "      <tbody\n";

static const char html_mid[] =
    "      </tbody>\n"
    "    </table>\n"
    "    <h2>Environment Variables</h2>"
    "    <table>\n"
    "      <thead>\n"
    "        <tr>\n"
    "          <th>Name</th>\n"
    "          <th>Value</th>\n"
    "        </tr>\n"
    "      </thead>\n"
    "      <tbody\n";

static const char html_tail[] =
    "      </tbody>\n"
    "    </table>\n"
    "  </body>\n"
    "</html>\n";

int cgi_main(struct timespec *start, cgi_query_t *query) {
    const char *fail;
    int fcount, np;
    cgi_param_t *p;
    char **env_ptr, *env_entry, *sep;

    if ((fail = cgi_get_param(query, "fail"))) {
	fcount = atoi(fail);
	if (fcount == 0)
	    log_msg("Failure mode with count=0");
	else
	    while (fcount--)
		log_msg("failure, count=%d", fcount);
	return 1;
    } else {
	cgi_out_text(html_head, sizeof(html_head)-1);
	np = query->nparam;
	p = query->params;
	while (np--) {
	    cgi_out_str("<tr><td>");
	    cgi_out_htmlesc(p->name);
	    cgi_out_str("</td><td>");
	    cgi_out_htmlesc(p->value);
	    cgi_out_str("</td></tr>\n");
	}
	cgi_out_text(html_mid, sizeof(html_mid)-1);
	for (env_ptr = environ; (env_entry = *env_ptr++); ) {
	    cgi_out_str("<tr><td>");
	    if ((sep = strchr(env_entry, '='))) {
		*sep = '\0';
		cgi_out_htmlesc(env_entry);
		*sep = '=';
		cgi_out_str("</td><td>");
		cgi_out_htmlesc(sep+1);
	    }
	    else
		cgi_out_htmlesc(env_entry);
	    cgi_out_str("</td></tr>\n");
	}
	cgi_out_text(html_tail, sizeof(html_tail)-1);
	return 0;
    }
}
