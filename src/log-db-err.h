#ifndef LOG_DB_ERR_H
#define LOG_DB_ERR_H

#include <libpq-fe.h>

extern void log_db_err(PGconn *conn, const char *msg, ...);

#endif
