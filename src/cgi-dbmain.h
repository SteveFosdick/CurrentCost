#ifndef CGI_DB_MAIN_H
#define CGI_DB_MAIN_H

#include "cgi-main.h"

#include <libpq-fe.h>

extern int cgi_db_main(struct timespec *start, cgi_query_t *query, PGconn *dbconn);

#endif
