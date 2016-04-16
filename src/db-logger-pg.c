#include "cc-common.h"
#include "db-logger.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <libpq-fe.h>

#define NUM_COLS 4
#define TIME_STAMP_SIZE 24

struct _db_logger_t {
    const char     *conninfo;
    PGconn         *conn;
    struct timeval lost_from;
    struct timeval lost_to;
    int            lost_count;
};

static const char power_sql[] =
    "INSERT INTO power (time_stamp, sensor, temperature, watts) "
    "VALUES ($1, $2, $3, $4)";

static const char pulse_sql[] =
    "INSERT INTO pulse (time_stamp, sensor, temperature, pulses) "
    "VALUES ($1, $2, $3, $4)";

static void log_db_err(PGconn *conn, const char *msg, ...) {
    va_list ap;
    struct timeval tv;
    struct tm *tp;
    char stamp[24];
    const char *ptr, *end;

    gettimeofday(&tv, NULL);
    tp = localtime(&tv.tv_sec);
    strftime(stamp, sizeof stamp, log_hdr1, tp);
    fprintf(stderr, log_hdr2, stamp, (int) (tv.tv_usec / 1000), prog_name);
    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    putc('\n', stderr);
    va_end(ap);
    ptr = PQerrorMessage(conn);
    while ((end = strchr(ptr, '\n'))) {
	fprintf(stderr, log_hdr2, stamp, (int) (tv.tv_usec / 1000), prog_name);
	fwrite(ptr, end - ptr + 1, 1, stderr);
	ptr = end + 1;
    }
}

static ExecStatusType prepare_statements(PGconn *conn) {
    PGresult *res;
    ExecStatusType code;

    if ((res = PQprepare(conn, "power", power_sql, 0, NULL))) {
	if ((code = PQresultStatus(res)) == PGRES_COMMAND_OK) {
	    PQclear(res);
	    if ((res = PQprepare(conn, "pulse", pulse_sql, 0, NULL))) {
		if ((code = PQresultStatus(res)) == PGRES_COMMAND_OK) {
		    PQclear(res);
		    return code;
		} else {
		    log_db_err(conn, "error preparing pulse SQL");
		    PQclear(res);
		}
	    } else {
		log_syserr("out of memory preparing pulse SQL");
		code = PGRES_FATAL_ERROR;
	    }
	} else {
	    log_db_err(conn, "error preparing power SQL");
	    PQclear(res);
	}
    } else {
	log_syserr("out of memory preparing power SQL");
	code = PGRES_FATAL_ERROR;
    }
    return code;
}

extern db_logger_t *db_logger_new(const char *db_conn) {
    db_logger_t *db_logger;
    PGconn *conn;

    if ((db_logger = malloc(sizeof(db_logger_t)))) {
	if ((conn = PQconnectdb(db_conn))) {
	    if (PQstatus(conn) == CONNECTION_OK) {
		if (prepare_statements(conn) == PGRES_COMMAND_OK) {
		    memset(db_logger, 0, sizeof(db_logger_t));
		    db_logger->conninfo = db_conn;
		    db_logger->conn = conn;
		    return db_logger;
		}
	    } else
		log_db_err(conn, "unable to connect to database '%s'", db_conn);
	    PQfinish(conn);
	} else
	    log_syserr("out of memory connecting to database");
	free(db_logger);
    } else
	log_syserr("out of memory create db-logger");
    return NULL;
}

extern void db_logger_free(db_logger_t *db_logger) {
    if (db_logger) {
	if (db_logger->conn)
	    PQfinish(db_logger->conn);
	free(db_logger);
    }
}

static int format_timestamp(struct timeval *when, char *stamp) {
    struct tm  *tp;
    
    tp = gmtime(&when->tv_sec);
    return snprintf(stamp, TIME_STAMP_SIZE,
		    "%04d-%02d-%02d %02d:%02d:%02d.%06d+00",
		    tp->tm_year + 1900, tp->tm_mon, tp->tm_mday,
		    tp->tm_hour, tp->tm_min, tp->tm_sec, (int)when->tv_usec);
}

static inline void report_losses(db_logger_t *db_logger) {
    char from_stamp[TIME_STAMP_SIZE], to_stamp[TIME_STAMP_SIZE];

    format_timestamp(&db_logger->lost_from, from_stamp);
    if (db_logger->lost_count == 1) {
	log_msg("1 entry lost at %s (%d)",
		from_stamp, db_logger->lost_from.tv_sec);
    } else {
	format_timestamp(&db_logger->lost_to, to_stamp);
	log_msg("%d entries lost from %s (%d) to %s (%d)",
		db_logger->lost_count, from_stamp,
		db_logger->lost_from.tv_sec, to_stamp,
		db_logger->lost_to.tv_sec);
    }
    db_logger->lost_count = 0;
}

static void retry_exec(db_logger_t *db_logger, const char *stmt,
		       const char **values, int *lengths) {
    PGresult *res;

    PQreset(db_logger->conn);
    if (PQstatus(db_logger->conn) != CONNECTION_OK) {
	if (db_logger->lost_count == 0)
	    log_db_err(db_logger->conn, "reconnection failed");
	db_logger->lost_count++;
    }
    else {
	log_msg("reconnected to database");
	if (db_logger->lost_count > 0)
	    report_losses(db_logger);
	if (prepare_statements(db_logger->conn) == PGRES_COMMAND_OK) {
	    res = PQexecPrepared(db_logger->conn, stmt, NUM_COLS, values, lengths, NULL, 0);
	    if (res) {
		if (PQresultStatus(res) != PGRES_COMMAND_OK)
		    log_db_err(db_logger->conn, "second failure executing %s insert statment", stmt);
		PQclear(res);
	    } else
		log_syserr("out of memory re-executing %s SQL", stmt);
	}
    }
}

static inline void exec_stmt(db_logger_t *db_logger, const char *stmt,
			     const char **values, int *lengths,
			     struct timeval *when) {
    char tstamp[TIME_STAMP_SIZE];
    PGresult *res;

    values[0] = tstamp;
    lengths[0] = format_timestamp(when, tstamp);
    if (PQstatus(db_logger->conn) == CONNECTION_OK) {
	res = PQexecPrepared(db_logger->conn, stmt, NUM_COLS, values, lengths, NULL, 0);
	if (res) {
	    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		log_db_err(db_logger->conn, "unable to execute %s insert statment", stmt);
		if (PQstatus(db_logger->conn) != CONNECTION_OK) {
		    log_msg("database connection lost - attemping reconnect");
		    db_logger->lost_from = *when;
		    retry_exec(db_logger, stmt, values, lengths);
		}
	    }
	    PQclear(res);
	}
	else
	    log_syserr("out of memory executing %s SQL", stmt);
    } else {
	db_logger->lost_to = *when;
	retry_exec(db_logger, stmt, values, lengths);
    }
}

extern void db_logger_line(db_logger_t *db_logger,
			   struct timeval *when, char *line, char *line_end) {
    char *start, *end;
    const char *stmt;
    const char *values[NUM_COLS];
    int        lengths[NUM_COLS];

    *line_end = '\0';
    if ((start = strstr((char *)line, "<tmpr>"))) {
	start += 6;
	if ((end = strchr(start, '<'))) {
	    *end = '\0';
	    values[2] = start;
	    lengths[2] = end - start;
	    if ((start = strstr(end+1, "<id>"))) {
		start += 4;
		if ((end = strchr(start, '<'))) {
		    *end = '\0';
		    values[1] = start;
		    lengths[1] = end - start;
		    if ((start = strstr(end+1, "<type>"))) {
			start += 6;
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
			    *end = '\0';
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
