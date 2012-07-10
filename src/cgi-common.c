#include "cc-common.h"
#include "cgi-common.h"

#include <unistd.h>

const char *sensor_names[] = {
    "Whole House",
    "Computers",
    "TV & Audio",
    "Utility",
    "Sensor 4",
    "Sensor 5",
    "Sensor 6",
    "Sensor 7",
    "Sensor 8",
    "Sensor 9",
    "Sensor 10"
};

static const char html_top[] =
    "\n"
    "<html>\n"
    "  <head>\n"
    "    <meta name=\"HandheldFriendly\" content=\"true\"/>\n"
    "    <meta name=\"viewport\" content=\"target-densitydpi=device-dpi\"/>\n"
    "    <meta name=\"viewport\" content=\"initial-scale=2\"/>\n"
    "    <link rel=\"stylesheet\" type=\"text/css\" href=\"/currentcost/currentcost.css\"/>\n";

void send_html_top(FILE *ofp) {
    fwrite(html_top, sizeof(html_top)-1, 1, ofp);
}

static const char html_tail[] =
    "  </body>\n"
    "</html>\n";

void send_html_tail(FILE *ofp) {
    fwrite(html_tail, sizeof(html_tail)-1, 1, ofp);
}

int main(int argc, char **argv) {
    if (chdir(default_dir) == 0)
	return cgi_main(argc, argv);
    log_syserr("unable to chdir to '%s'", default_dir);
    return 1;
}
