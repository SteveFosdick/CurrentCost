#ifndef CGI_MAIN_H
#define CGI_MAIN_H

#include <stdio.h>

typedef struct {
    char *name;
    char *value;
} cgi_param_t;

typedef struct {
    char *data;
    unsigned nparam;
    cgi_param_t *params;
} cgi_query_t;

extern const char base_url[];
extern void send_html_top(FILE * ofp);
extern void send_html_tail(FILE * ofp);

extern void cgi_html_esc_out(const char *str, FILE *ofp);
extern char *cgi_urldec(char *dest, const char *src);
extern char *cgi_get_param(cgi_query_t *query, const char *name);
extern int cgi_main(cgi_query_t *query);

#endif
