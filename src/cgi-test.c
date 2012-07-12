#include "cgi-common.h"

const char prog_name[] = "cgi-test";

int cgi_main(int argc, char **argv) {
    cgi_query_t *q;

    if ((q = cgi_get_query()))
	printf("a='%s',ba='%s'\n",
	       cgi_get_param(q, "a"),
	       cgi_get_param(q, "ba"));
    else
	fputs("failed\n", stderr);
    return 0;
}
