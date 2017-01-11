#ifndef CGI_MAIN_H
#define CGI_MAIN_H

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

typedef struct {
    char *name;
    char *value;
} cgi_param_t;

typedef struct {
    char *data;
    unsigned nparam;
    cgi_param_t *params;
} cgi_query_t;

extern const char prog_name[];

extern void log_msg(const char *msg, ...);
extern void log_syserr(const char *msg, ...);

extern char *cgi_urldec(char *dest, const char *src);
extern char *cgi_get_param(cgi_query_t *query, const char *name);
extern int cgi_main(struct timespec *start, cgi_query_t *query, FILE *cgi_str);

#endif
