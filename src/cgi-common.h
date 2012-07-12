#ifndef CGI_COMMON_H
#define CGI_COMMON_H

#include <stdio.h>

typedef struct {
    char         *name;
    char         *value;
} cgi_param_t;

typedef struct {
    char         *data;
    unsigned     nparam;
    cgi_param_t  *params;
} cgi_query_t;

extern const char *sensor_names[];
extern void send_html_top(FILE *ofp);
extern void send_html_tail(FILE *ofp);

extern int cgi_main(int argc, char **argv);
char *cgi_urldec(char *dest, const char *src);
extern cgi_query_t *cgi_get_query(void);
extern char *cgi_get_param(cgi_query_t *query, const char *name);

#endif
