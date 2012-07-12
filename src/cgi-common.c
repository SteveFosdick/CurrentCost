#include "cc-common.h"
#include "cgi-common.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
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

static char *method_get(void) {
    char *data;
    
    if ((data = getenv("QUERY_STRING")) == NULL)
	log_msg("environment variable QUERY_STRING not set");
    else if ((data = strdup(data)) == NULL)
	log_syserr("unable to allocate query string");
    return data;
}

static char *method_post(void) {
    char *data, *conlen, *ptr;
    ssize_t consiz, togo, nread;
    
    if ((conlen = getenv("CONTENT_LENGTH")) == NULL) {
	log_msg("environment variable CONTENT_LENGTH not set");
        data = NULL;
    } else if ((consiz = atoi(conlen)) <= 0) {
	log_msg("environment variable CONTENT_LENGTH has an invalid value");
	data = NULL;
    } else if ((data = malloc(consiz + 1)) == NULL)
        log_syserr("unable to allocate query buffer");
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

char *cgi_urldec(char *dest, const char *src) {
    char *eqs;
    int  ch, nc;

    for (eqs = NULL; (ch = *src++) != '\0'; ) {
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

static int split_fields(char *data, cgi_query_t *query) {
    int         status, nparam, ch;
    char        *ptr, *nxt, *eqs;
    cgi_param_t *pp;

    /* count parameters */
    for (nparam = 1, ptr = data; (ch = *ptr++); )
        if (ch == '&')
            nparam++;

    if (((query->params = malloc(nparam * sizeof(cgi_param_t))))) {
	status = 0;
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
    } else {
	log_msg("unable to allocate query paramaters");
	status = -1;
    }
    return status;
}

cgi_query_t *cgi_get_query(void) {
    cgi_query_t *query;
    char        *method;
    char        *data;

    if ((query = malloc(sizeof(cgi_query_t)))) {
	if ((method = getenv("REQUEST_METHOD")) == NULL) {
	    log_msg("environment variable REQUEST_METHOD not set");
	    data = NULL;
	} else if (strcasecmp(method, "GET") == 0)
	    data = method_get();
	else if (strcasecmp(method, "POST") == 0)
	    data = method_post();
	else {
	    log_msg("request method %s not supported", method);
	    data = NULL;
	}
	if (data) {
	    query->data = data;
	    if (*data == '\0') {
		query->nparam = 0;
		query->params = NULL;
		return query;
	    } else if (split_fields(data, query) == 0)
		return query;
	    free(data);
	}
	free(query);
    } else
	log_msg("unable to allocate query");
    return NULL;
}

char *cgi_get_param(cgi_query_t *query, const char *name) {
    cgi_param_t *pp, *pe;
    char *ptr;
    
    pp = query->params;
    pe = pp + query->nparam;
    while (pp < pe) {
        if (strcasecmp(pp->name, name) == 0) {
            for (ptr = pp->value; isspace(*ptr); ptr++)
                ;
            if (*ptr == '\0')
                return NULL;
            else
                return ptr;
        }
        pp++;
    }
    return NULL;
}
