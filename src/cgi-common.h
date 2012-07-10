#ifndef CGI_COMMON_H
#define CGI_COMMON_H

#include <stdio.h>

extern const char *sensor_names[];
extern void send_html_top(FILE *ofp);
extern void send_html_tail(FILE *ofp);

extern int cgi_main(int argc, char **argv);

#endif
