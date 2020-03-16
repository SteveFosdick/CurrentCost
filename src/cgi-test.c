#include "cgi-main.h"
#include "cc-html.h"
#include <stdlib.h>
#include <string.h>

extern char **environ;

const char prog_name[] = "cgi-test";

static const char html_head[] =
    "Content-Type: text/html; charset=utf-8\n"
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

int cgi_main(struct timespec *start, cgi_query_t *query, FILE *cgi_str)
{
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
    }
    else {
        fwrite(html_head, sizeof(html_head) - 1, 1, cgi_str);
        np = query->nparam;
        p = query->params;
        while (np--) {
            html_puts("<tr><td>", cgi_str);
            html_esc(p->name, cgi_str);
            html_puts("</td><td>", cgi_str);
            html_esc(p->value, cgi_str);
            html_puts("</td></tr>\n", cgi_str);
        }
        fwrite(html_mid, sizeof(html_mid) - 1, 1, cgi_str);
        for (env_ptr = environ; (env_entry = *env_ptr++);) {
            html_puts("<tr><td>", cgi_str);
            if ((sep = strchr(env_entry, '='))) {
                *sep = '\0';
                html_esc(env_entry, cgi_str);
                *sep = '=';
                html_puts("</td><td>", cgi_str);
                html_esc(sep + 1, cgi_str);
            }
            else
                html_esc(env_entry, cgi_str);
            html_puts("</td></tr>\n", cgi_str);
        }
        fwrite(html_tail, sizeof(html_tail) - 1, 1, cgi_str);
        return 0;
    }
}
