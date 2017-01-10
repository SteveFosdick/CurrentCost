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
#include <obstack.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define obstack_chunk_alloc malloc
#define obstack_chunk_free  free

static const char cgi_errhdr[] =
    "Content-Type: text/plain\n"
    "Status: 500 Internal Server Error\n"
    "\n";

static const char cgi_memerr[] =
    "Out of memory attempting to allocate an obstack chunk\n";

static void obstack_alloc_failed() {
    fwrite(cgi_memerr, sizeof(cgi_memerr)-1, 1, stderr);
    fwrite(cgi_errhdr, sizeof(cgi_errhdr)-1, 1, stdout);
    fwrite(cgi_memerr, sizeof(cgi_memerr)-1, 1, stdout);
    exit(1);
}

static struct obstack log_obs;
static struct obstack cgi_obs;

struct list_item {
    struct list_item *next;
    size_t           size;
    const char       *data;
};

struct list_item *out_head, *out_tail, *log_head, *log_tail;

static void enlist(struct list_item **head, struct list_item **tail, char *data, size_t size) {
    struct list_item *new_item, *ptr;

    new_item = obstack_alloc(&log_obs, sizeof(struct list_item));
    new_item->next = NULL;
    new_item->size = size;
    new_item->data = data;
    if ((ptr = *tail))
	ptr->next = new_item;
    else
	*head = new_item;
    *tail = new_item;
}

const char log_hdr1[] = "%d/%m/%Y %H:%M:%S";
const char log_hdr2[] = "%s.%03d %s: ";

static void log_begin(const char *msg, va_list ap)
{
    struct timespec ts;
    struct tm *tp;
    char stamp[24];

    clock_gettime(CLOCK_REALTIME, &ts);
    tp = localtime(&ts.tv_sec);
    strftime(stamp, sizeof stamp, log_hdr1, tp);
    obstack_printf(&log_obs, log_hdr2, stamp, (int)(ts.tv_nsec / 1000000), prog_name);
    obstack_vprintf(&log_obs, msg, ap);
}

static void log_end() {
    size_t size;
    char   *data;

    size = obstack_object_size(&log_obs);
    data = obstack_finish(&log_obs);
    fwrite(data, size, 1, stderr);
    enlist(&log_head, &log_tail, data, size);
}

void log_msg(const char *msg, ...)
{
    va_list ap;

    va_start(ap, msg);
    log_begin(msg, ap);
    va_end(ap);
    obstack_1grow(&log_obs, '\n');
    log_end();
}

void log_syserr(const char *msg, ...)
{
    const char *syserr;
    va_list ap;

    syserr = strerror(errno);
    va_start(ap, msg);
    log_begin(msg, ap);
    va_end(ap);
    obstack_printf(&log_obs, ": %s\n", syserr);
    log_end();
}

static void send_output(struct list_item *list) {
    while (list) {
	fwrite(list->data, list->size, 1, stdout);
	list = list->next;
    }
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

static void split_fields(char *data, cgi_query_t *query)
{
    int nparam, ch;
    char *ptr, *nxt, *eqs;
    cgi_param_t *pp;

    if (*data == '\0') {
	query->nparam = 0;
	query->params = NULL;
    }
    else {
	/* count parameters */
	for (nparam = 1, ptr = data; (ch = *ptr++);)
	    if (ch == '&')
		nparam++;

	query->params = obstack_alloc(&cgi_obs, nparam * sizeof(cgi_param_t));
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
    }
}

static char *method_get(void)
{
    char *data;

    if ((data = getenv("QUERY_STRING")) == NULL) {
        log_msg("environment variable QUERY_STRING not set");
	return NULL;
    }
    return obstack_copy0(&cgi_obs, data, strlen(data));
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
    } else {
	data = obstack_alloc(&cgi_obs, consiz + 1);
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
            obstack_free(&cgi_obs, data);
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
    obstack_alloc_failed_handler = &obstack_alloc_failed;
    obstack_init(&log_obs);
    obstack_init(&cgi_obs);

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
	split_fields(data, &query);
	status = cgi_main(&start, &query);
    }
    if (status == 0)
	send_output(out_head);
    else {
	fwrite(cgi_errhdr, sizeof(cgi_errhdr)-1, 1, stdout);
	send_output(log_head);
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

void cgi_out_text(const char *str, size_t len) {
    char *data = obstack_copy0(&cgi_obs, str, len);
    enlist(&out_head, &out_tail, data, len);
}

static void cgi_out_end() {
    size_t size;
    char   *data;

    size = obstack_object_size(&cgi_obs);
    data = obstack_finish(&cgi_obs);
    enlist(&out_head, &out_tail, data, size);
}

void cgi_out_htmlesc(const char *str) {
    int ch;

    while ((ch = *str++)) {
	switch(ch) {
	case '<':
	    obstack_grow(&cgi_obs, "&lt;", 4);
	    break;
	case '>':
	    obstack_grow(&cgi_obs, "&gt;", 4);
	    break;
	case '&':
	    obstack_grow(&cgi_obs, "&amp;", 5);
	    break;
	default:
	    obstack_1grow(&cgi_obs, ch);
	}
    }
    cgi_out_end();
}

void cgi_out_printf(const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    obstack_vprintf(&cgi_obs, fmt, ap);
    va_end(ap);
    cgi_out_end();
}

void cgi_out_ch(int ch) {
    obstack_1grow(&cgi_obs, ch);
    cgi_out_end();
}
