/*
 * cc-html
 *
 * This provides common URLs, HTML and functions for the current-cost project.
 */

#include "cc-defs.h"
#include "cc-html.h"

const char default_dir[] = DEFAULT_DIR;
const char xml_file[] = XML_FILE;
const char date_iso[] = DATE_ISO;
const char base_url[] = BASE_URL;

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

void html_send_top(FILE *fp)
{
    fwrite(html_top, sizeof(html_top) - 1, 1, fp);
}

/* *INDENT-OFF* */
static const char html_tail[] =
    "  </body>\n"
    "</html>\n";
/* *INDENT-ON* */

void html_send_tail(FILE *fp)
{
    fwrite(html_tail, sizeof(html_tail) - 1, 1, fp);
}

void html_esc(const char *data, FILE *fp)
{
    int ch;

    while ((ch = *data++)) {
        switch (ch) {
            case '<':{
                    const char html_lt[] = "&lt;";
                    fwrite(html_lt, sizeof(html_lt) - 1, 1, fp);
                }
                break;
            case '>':{
                    const char html_gt[] = "&gt;";
                    fwrite(html_gt, sizeof(html_gt) - 1, 1, fp);
                }
                break;
            case '&':{
                    const char html_amp[] = "&amp;";
                    fwrite(html_amp, sizeof(html_amp) - 1, 1, fp);
                }
                break;
            default:
                putc(ch, fp);
        }
    }
}
