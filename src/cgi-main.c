/*
 * cgi-main
 *
 * This is a ready-made main program for a CGI program, providing a framework
 * in which output and error messages are buffered such that one or the
 * other is sent to the web server, but never both, and handling the reading
 * and parsing of the query string.  Some generally useful functions for
 * working with CGI are also provided.
 */

#define _GNU_SOURCE

#include "cgi-main.h"

#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const char cgi_errhdr[] =
    "Content-Type: text/plain\n"
    "Status: 500 Internal Server Error\n"
    "\n";

const char log_hdr1[] = "%d/%m/%Y %H:%M:%S";
const char log_hdr2[] = "%s.%03d %s: ";

/* GNU stdio string streams for log and CGI output */

static FILE   *log_str, *cgi_str;
static char   *log_ptr, *cgi_ptr;
static size_t log_size, cgi_size;

static void log_begin(const char *msg, va_list ap)
{
    struct timespec ts;
    struct tm *tp;
    char stamp[24];

    clock_gettime(CLOCK_REALTIME, &ts);
    tp = localtime(&ts.tv_sec);
    strftime(stamp, sizeof stamp, log_hdr1, tp);
    fprintf(log_str, log_hdr2, stamp, (int)(ts.tv_nsec / 1000000), prog_name);
    vfprintf(log_str, msg, ap);
}

void log_msg(const char *msg, ...)
{
    va_list ap;

    va_start(ap, msg);
    log_begin(msg, ap);
    va_end(ap);
    putc('\n', log_str);
}

void log_syserr(const char *msg, ...)
{
    const char *syserr;
    va_list ap;

    syserr = strerror(errno);
    va_start(ap, msg);
    log_begin(msg, ap);
    va_end(ap);
    fprintf(log_str, ": %s\n", syserr);
}

char *cgi_urldec(char *dest, const char *src)
{
    char *eqs;
    int ch, nc;

    for (eqs = NULL; (ch = *src++) != '\0';) {
        if (ch == '+')
            *dest++ = ' ';
        else if (ch == '%' && isxdigit(src[0]) && isxdigit(src[1])) {
            ch = *src++;
            if (isupper(ch))
                nc = (10 + (ch - 'A')) << 4;
            else if (islower(ch))
                nc = (10 + (ch - 'a')) << 4;
            else
                nc = (ch - '0') << 4;
            ch = *src++;
            if (isupper(ch))
                nc |= 10 + (ch - 'A');
            else if (islower(ch))
                nc |= 10 + (ch - 'a');
            else
                nc |= ch - '0';
            *dest++ = nc;
        } else {
            if (ch == '=' && eqs == NULL)
                eqs = dest;
            *dest++ = ch;
        }
    }
    *dest = '\0';
    return eqs;
}

static int split_params(char *data, cgi_query_t *query)
{
    int status, nparam, ch;
    char *ptr, *nxt, *eqs;
    cgi_param_t *pp;

    if (*data == '\0') {
	query->nparam = 0;
	query->params = NULL;
	status = 0;
    }
    else {
	/* count parameters */
	for (nparam = 1, ptr = data; (ch = *ptr++);)
	    if (ch == '&')
		nparam++;

	if ((query->params = malloc(nparam * sizeof(cgi_param_t)))) {
	    /* step through fields. */
	    query->nparam = nparam;
	    pp = query->params;
	    for (ptr = data; ptr; ptr = nxt) {
		if ((nxt = strchr(ptr, '&')))
		    *nxt++ = '\0';
		if ((eqs = cgi_urldec(ptr, ptr)))
		    *eqs++ = '\0';
		pp->name = ptr;
		pp->value = eqs;
		pp++;
	    }
	    status = 0;
	} else {
	    log_syserr("unable to allocate space for CGI parameters");
	    status = 2;
	}
    }
    return status;
}

static char *method_get(void)
{
    char *data;

    if ((data = getenv("QUERY_STRING")) == NULL)
        log_msg("environment variable QUERY_STRING not set");
    else if ((data = strdup(data)) == NULL)
	log_syserr("unable to copy query string");
    return data;
}

static char *method_post(void)
{
    char *data, *conlen, *ptr;
    ssize_t consiz, togo, nread;

    if ((conlen = getenv("CONTENT_LENGTH")) == NULL) {
        log_msg("environment variable CONTENT_LENGTH not set");
        data = NULL;
    } else if ((consiz = atoi(conlen)) <= 0) {
        log_msg("environment variable CONTENT_LENGTH has an invalid value");
        data = NULL;
    } else if ((data = malloc(consiz + 1)) == NULL)
	log_msg("unable to allocate space for CGI input");
    else {
        togo = consiz;
        ptr = data;
        while (togo > 0 && (nread = read(STDIN_FILENO, ptr, togo)) > 0) {
            togo -= nread;
            ptr += nread;
        }
        if (togo > 0) {
            if (nread < 0)
                log_syserr("read error on stdin");
            else
                log_msg("input form was truncated");
            free(data);
            data = NULL;
        } else
            *ptr = '\0';
    }
    return data;
}

int main(int argc, char **argv) {
    struct timespec start;
    int status = 1;
    const char *method;
    char *data;
    cgi_query_t query;

    clock_gettime(CLOCK_REALTIME, &start);
    log_str = open_memstream(&log_ptr, &log_size);
    cgi_str = open_memstream(&cgi_ptr, &cgi_size);

    if (setlocale(LC_ALL, "en_GB.utf8") == NULL)
	log_syserr("unable to set locale to en_GB.utf8");

    if ((method = getenv("REQUEST_METHOD")) == NULL) {
	log_msg("environment variable REQUEST_METHOD not set");
	data = NULL;
    }
    else if (strcasecmp(method, "GET") == 0)
	data = method_get();
    else if (strcasecmp(method, "POST") == 0)
	data = method_post();
    else {
	log_msg("request method %s not supported", method);
	data = NULL;
    }
    if (data) {
	if ((status = split_params(data, &query)) == 0)
	    status = cgi_main(&start, &query, cgi_str);
    }
    fflush(log_str);
    if (log_size > 0)
	write(2, log_ptr, log_size);
    if (status == 0) {
	fflush(cgi_str);
	write(1, cgi_ptr, cgi_size);
    }
    else {
	fwrite(cgi_errhdr, sizeof(cgi_errhdr)-1, 1, stdout);
	fwrite(log_ptr, log_size, 1, stdout);
	fflush(stdout);
    }
    return status;
}

char *cgi_get_param(cgi_query_t * query, const char *name)
{
    cgi_param_t *pp, *pe;
    char *ptr;

    pp = query->params;
    pe = pp + query->nparam;
    while (pp < pe) {
        if (strcasecmp(pp->name, name) == 0) {
            for (ptr = pp->value; isspace(*ptr); ptr++);
            if (*ptr == '\0')
                return NULL;
            else
                return ptr;
        }
        pp++;
    }
    return NULL;
}
