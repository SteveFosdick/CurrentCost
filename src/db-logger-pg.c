#include "cc-common.h"
#include "db-logger.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <libpq-fe.h>

#define NUM_COLS  4
#define MAX_DATA  (NUM_COLS * 20)
#define TIME_STAMP_SIZE 24
#define RETRY_WAIT 30

typedef struct sample sample_t;

struct sample {
    sample_t *next;
    struct timeval when;
    union {
	const char *stmt;
	char *data_ptr;
    } ptr;
    int lengths[NUM_COLS];
    const char *values[NUM_COLS];
    char data[MAX_DATA];
};

struct _db_logger_t {
    PGconn          *conn;
    pthread_t       thread;
    pthread_mutex_t lock;
    pthread_cond_t  wait_data;
    sample_t        *head;
    sample_t        *tail;
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

static void retry_exec(db_logger_t *db_logger, sample_t *smp) {
    PGresult *res;

    while (PQstatus(db_logger->conn) != CONNECTION_OK) {
	sleep(RETRY_WAIT);
	PQreset(db_logger->conn);
    }
    log_msg("reconnected to database");
    if (prepare_statements(db_logger->conn) == PGRES_COMMAND_OK) {
	res = PQexecPrepared(db_logger->conn, smp->ptr.stmt, NUM_COLS,
			     smp->values, smp->lengths, NULL, 0);
	if (res) {
	    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
		log_db_err(db_logger->conn,
		       "retry failed for %s insert statment", smp->ptr.stmt);
	    }
	    PQclear(res);
	} else
	    log_syserr("unable to allocate result for %s insert statement");
    }
}

static void db_exec(db_logger_t *db_logger, sample_t *smp) {
    struct tm  *tp;
    char tstamp[TIME_STAMP_SIZE];
    PGresult *res;

    smp->values[0] = tstamp;
    tp = gmtime(&smp->when.tv_sec);
    smp->lengths[0] = snprintf(tstamp, sizeof(tstamp),
			       "%04d-%02d-%02d %02d:%02d:%02d.%06d+00",
			       tp->tm_year + 1900, tp->tm_mon + 1, tp->tm_mday,
			       tp->tm_hour, tp->tm_min, tp->tm_sec,
			       (int)(smp->when.tv_usec));
    res = PQexecPrepared(db_logger->conn, smp->ptr.stmt, NUM_COLS,
			 smp->values, smp->lengths, NULL, 0);
    if (res) {
	if (PQresultStatus(res) != PGRES_COMMAND_OK) {
	    log_db_err(db_logger->conn,
		       "unable to execute %s insert statment", smp->ptr.stmt);
	    if (PQstatus(db_logger->conn) != CONNECTION_OK) {
		log_msg("database connection lost - attemping reconnect");
		retry_exec(db_logger, smp);
	    }
	}
	PQclear(res);
    }
    else
	log_syserr("out of memory executing %s SQL", smp->ptr.stmt);
}

static void *db_thread(void *ptr) {
    db_logger_t *db_logger = ptr;
    sample_t *smp;

    for (;;) {
	pthread_mutex_lock(&db_logger->lock);
	while (db_logger->head == NULL)
	    pthread_cond_wait(&db_logger->wait_data, &db_logger->lock);
	smp = db_logger->head;
	db_logger->head = smp->next;
	if (smp->next == NULL)
	    db_logger->tail = NULL;
	pthread_mutex_unlock(&db_logger->lock);
	if (smp->ptr.stmt == NULL)
	    break;
	db_exec(db_logger, smp);
	free(smp);
    }
    PQfinish(db_logger->conn);
    return NULL;
}

static char *parse_elem(sample_t *smp, int index, const char *pat,
			char *str, const char *end) {
    const char *pat_ptr;
    char *str_ptr, *dst_start, *dst_ptr, *dst_max;
    int ch;

    while (str < end) {
	str_ptr = str;
	pat_ptr = pat;
	ch = *pat_ptr++;
	while (str_ptr < end && ch == *str_ptr) {
	    ch = *pat_ptr++;
	    str_ptr++;
	}
	if (!ch) {
	    dst_ptr = dst_start = smp->ptr.data_ptr;
	    dst_max = smp->data + MAX_DATA;
	    while (str_ptr < end && dst_ptr < dst_max) {
		if ((ch = *str_ptr++) == '<') {
		    *dst_ptr++ = '\0';
		    smp->values[index] = dst_start;
		    smp->lengths[index] = dst_ptr - dst_start;
		    smp->ptr.data_ptr = dst_ptr;
		    return str_ptr;
		}
		*dst_ptr++ = ch;
	    }
	    break;
	}
	str++;
    }
    return NULL;
}

static void enqueue(db_logger_t *db_logger, sample_t *smp, const char *stmt) {
    smp->ptr.stmt = stmt;
    smp->next = NULL;
    pthread_mutex_lock(&db_logger->lock);
    if (db_logger->tail == NULL)
	db_logger->head = smp;
    else	
	db_logger->tail->next = smp;
    db_logger->tail = smp;
    pthread_cond_signal(&db_logger->wait_data);
    pthread_mutex_unlock(&db_logger->lock);
}

extern void db_logger_line(db_logger_t *db_logger,
			   struct timeval *when, char *line, char *line_end) {
    char *ptr;
    sample_t *smp;

    if ((smp = malloc(sizeof(sample_t)))) {
	smp->when = *when;
	smp->ptr.data_ptr = smp->data;
	if ((ptr = parse_elem(smp, 2, "<tmpr>", line, line_end))) {
	    if ((ptr = parse_elem(smp, 1, "<id>", ptr, line_end))) {
		if (parse_elem(smp, 3, "<watts>", ptr, line_end)) {
		    enqueue(db_logger, smp, "power");
		    return;
		} else if (parse_elem(smp, 3, "<imp>", ptr, line_end)) {
		    enqueue(db_logger, smp, "pulse");
		    return;
		}
	    }
	}
	free(smp);
    } else
	log_syserr("unable to allocate sample");
}

extern db_logger_t *db_logger_new(const char *db_conn) {
    db_logger_t *db_logger;
    PGconn      *conn;
    int         res;

    if ((db_logger = malloc(sizeof(db_logger_t)))) {
	if ((conn = PQconnectdb(db_conn))) {
	    if (PQstatus(conn) == CONNECTION_OK) {
		if (prepare_statements(conn) == PGRES_COMMAND_OK) {
		    memset(db_logger, 0, sizeof(db_logger_t));
		    db_logger->conn = conn;
		    res = pthread_create(&db_logger->thread, NULL,
					 db_thread, db_logger);
		    if (res == 0)
			return db_logger;
		    log_msg("unable to create database thread - %s",
			    strerror(res));
		}
	    } else
		log_db_err(conn, "unable to connect to database '%s'", db_conn);
	    PQfinish(conn);
	} else
	    log_syserr("unable to allocate datbase connection");
    } else
	log_syserr("unable to allocate db-logger");
    return NULL;
}
	
extern void db_logger_free(db_logger_t *db_logger) {
    sample_t smp;

    if (db_logger) {
	enqueue(db_logger, &smp, NULL); // send "EOF" message.
	pthread_join(db_logger->thread, NULL);
	free(db_logger);
    }
}
