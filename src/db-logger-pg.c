#include "cc-common.h"
#include "db-logger.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <libpq-fe.h>

#define NUM_COLS 4

struct _db_logger_t {
    const char *conninfo;
    PGconn     *conn;
};

static const char power_sql[] = "INSERT INTO powers VALUES ($1, $2)";
static const char pulse_sql[] = "INSERT INTO pulse VALUES ($1, $2)";

static void log_db_err(const char *what, const PGresult *res) {
    log_msg("error %s: %s", what, PQresultErrorMessage(res));
}

static inline PGconn *connect_and_prepare(db_logger_t *db_logger) {
    PGconn *conn;
    PGresult *res;

    if ((conn = PQconnectdb(db_logger->conninfo))) {
	if (PQstatus(conn) == CONNECTION_OK) {
	    if ((res = PQprepare(conn, "power", power_sql, 0, NULL))) {
		if (PQresultStatus(res) == PGRES_COMMAND_OK) {
		    PQclear(res);
		    if ((res = PQprepare(conn, "pulse", pulse_sql, 0, NULL))) {
			if (PQresultStatus(res) == PGRES_COMMAND_OK) {
			    PQclear(res);
			    db_logger->conn = conn;
			    return conn;
			} else {
			    log_db_err("preparing pulse SQL", res);
			    PQclear(res);
			}
		    } else
			log_syserr("out of memory preparing pulse SQL");
		} else {
		    log_db_err("preparing power SQL", res);
		    PQclear(res);
		}
	    } else
		log_syserr("out of memory preparing power SQL");
	} else {
	    log_msg("unable to connect to database '%s' - %s",
		    db_logger->conninfo, PQerrorMessage(conn));
	}
	PQfinish(conn);
    } else
	log_syserr("out of memory connecting to database");
    return NULL;
}    

static PGconn *connect_ok(db_logger_t *db_logger) {
    PGconn *conn;

    if ((conn = db_logger->conn) == NULL)
	conn = connect_and_prepare(db_logger);
    return conn;
}

extern db_logger_t *db_logger_new(const char *db_conn) {
    db_logger_t *db_logger;

    if ((db_logger = malloc(sizeof(db_logger_t)))) {
	db_logger->conninfo = db_conn;
	db_logger->conn = NULL;
    }
    return db_logger;
}

extern void db_logger_free(db_logger_t *db_logger) {
    if (db_logger) {
	if (db_logger->conn)
	    PQfinish(db_logger->conn);
	free(db_logger);
    }
}

static inline void exec_stmt(db_logger_t *db_logger, const char *stmt,
			     const char **values, int *lengths, struct timeval *when) {
    PGconn *conn;
    struct tm  *tp;
    char tstamp[24];

    if ((conn = connect_ok(db_logger))) {
	values[0] = tstamp;
	tp = gmtime(&when->tv_sec);
	lengths[0] = sprintf(tstamp, "%04d%02d%02dT%02d%02d%02d.%06d",
			     tp->tm_year + 1900, tp->tm_mon, tp->tm_mday,
			     tp->tm_hour,tp->tm_min, tp->tm_sec, (int)when->tv_usec);
	PGresult *res = PQexecPrepared(conn, stmt, NUM_COLS, values, lengths, NULL, 0);
	if (res) {
	    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		log_msg("error executing %s insert statement - %s", PQresultErrorMessage(res));
		if (PQstatus(conn) != CONNECTION_OK) {
		    log_msg("closing dead database connection");
		    PQfinish(conn);
		    db_logger->conn = NULL;
		}
	    }
	} else
	    log_syserr("out of memory preparing %s SQL", stmt);
    }
}

extern void db_logger_line(db_logger_t *db_logger,
			   struct timeval *when, char *line, char *line_end) {
    const char *start, *end, *stmt;
    const char *values[NUM_COLS];
    int        lengths[NUM_COLS];

    *line_end = '\0';
    if ((start = strstr((char *)line, "<tmpr>"))) {
	start += 6;
	if ((end = strchr(start, '<'))) {
	    values[2] = start;
	    lengths[2] = end - start;
	    if ((start = strstr(end, "<id>"))) {
		start += 4;
		if ((end = strchr(start, '<'))) {
		    values[1] = start;
		    lengths[1] = end - start;
		    if ((start = strstr(end, "<type>"))) {
			stmt = NULL;
			if (*start == '1') {
			    if ((start = strstr(start + 1, "<watts>"))) {
				start += 7;
				stmt = "power";
			    }
			} else if (*start == '2') {
			    if ((start = strstr(start + 1, "<imp>"))) {
				start += 5;
				stmt = "pulse";
			    }
			}
			if (stmt && (end = strchr(start, '<'))) {
			    values[3] = start;
			    lengths[3] = end - start;
			    exec_stmt(db_logger, stmt, values, lengths, when);
			}
		    }
		}
	    }
	}
    }
}
