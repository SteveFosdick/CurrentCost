/*
 * cc-html
 *
 * This provides common URLs, HTML and functions for the current-cost project.
 */

#include "cgi-main.h"
#include "cc-defs.h"

const char default_dir[] = DEFAULT_DIR;
const char xml_file[]    = XML_FILE;
const char base_url[]    = BASE_URL;

const char *sensor_names[] = {
    "Clamp Sensor",
    "Computers",
    "TV & Audio",
    "Utility",
    "Sensor 4",
    "Sensor 5",
    "Solar (Smoothed)",
    "Import (Smoothed)",
    "Solar (Pulse)",
    "Import (Pulse)",
    "Sensor 10"
};

/* *INDENT-OFF* */
static const char html_top[] =
    "\n<!DOCTYPE html>\n"
    "<html>\n"
    "  <head>\n"
    "    <meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\"> \n"
    "    <link rel=\"stylesheet\" type=\"text/css\" href=\"/currentcost/currentcost.css\">\n"
    "    <meta name=\"HandheldFriendly\" content=\"true\">\n"
    "    <meta name=\"viewport\" content=\"target-densitydpi=device-dpi\">\n";
/* *INDENT-ON* */

void send_html_top() {
    cgi_out_text(html_top, sizeof(html_top)-1);
}

/* *INDENT-OFF* */
static const char html_tail[] =
    "  </body>\n"
    "</html>\n";
/* *INDENT-ON* */

void send_html_tail() {
    cgi_out_text(html_tail, sizeof(html_tail)-1);
}
