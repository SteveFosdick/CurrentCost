#include "cc-common.h"
#include "db-logger.h"
#include "pg-common.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define RETRY_WAIT 30

struct _db_logger_t {
    PGconn *conn;
    pthread_t thread;
    pthread_mutex_t lock;
    pthread_cond_t wait_data;
    sample_t *head;
    sample_t *tail;
    struct timespec last;
};

static ExecStatusType db_setup(PGconn *conn)
{
    PGresult *res;
    ExecStatusType code;

    if (PQstatus(conn) != CONNECTION_OK) {
        log_db_err(conn, "problem with database connection");
        do {
            sleep(RETRY_WAIT);
            PQreset(conn);
        } while (PQstatus(conn) != CONNECTION_OK);
    }
    if ((res = PQexec(conn, "SET TIME ZONE UTC"))) {
        if ((code = PQresultStatus(res)) == PGRES_COMMAND_OK) {
            PQclear(res);
            if ((res = PQprepare(conn, "power", power_sql, 0, NULL))) {
                if ((code = PQresultStatus(res)) == PGRES_COMMAND_OK) {
                    PQclear(res);
                    if ((res = PQprepare(conn, "pulse", pulse_sql, 0, NULL))) {
                        if ((code = PQresultStatus(res)) == PGRES_COMMAND_OK) {
                            PQclear(res);
                            log_msg("database ready");
                            return code;
                        }
                        else {
                            log_db_err(conn, "error preparing pulse SQL");
                            PQclear(res);
                        }
                    }
                    else {
                        log_syserr("out of memory preparing pulse SQL");
                        code = PGRES_FATAL_ERROR;
                    }
                }
                else {
                    log_db_err(conn, "error preparing power SQL");
                    PQclear(res);
                }
            }
            else {
                log_syserr("out of memory preparing power SQL");
                code = PGRES_FATAL_ERROR;
            }
        }
        else {
            log_db_err(conn, "error setting timezone");
            PQclear(res);
        }
    }
    else {
        log_syserr("out of memory preparing time zone SQL");
        code = PGRES_FATAL_ERROR;
    }
    return code;
}

static void retry_exec(db_logger_t *db_logger, sample_t *smp)
{
    PGresult *res;

    if (db_setup(db_logger->conn) == PGRES_COMMAND_OK) {
        res = PQexecPrepared(db_logger->conn, smp->ptr.stmt, NUM_COLS, smp->values, smp->lengths, NULL, 0);
        if (res) {
            if (PQresultStatus(res) != PGRES_COMMAND_OK) {
                log_db_err(db_logger->conn, "retry failed for %s insert statment", smp->ptr.stmt);
            }
            PQclear(res);
        }
        else
            log_syserr("unable to allocate result for %s insert statement");
    }
}

static void db_exec(db_logger_t *db_logger, sample_t *smp)
{
    struct tm *tp;
    char tstamp[TIME_STAMP_SIZE];
    PGresult *res;

    /* avoid a primary key clash on timestamps */
    long this_usec = smp->when.tv_nsec / 1000;
    if (smp->when.tv_sec == db_logger->last.tv_sec) {
        long last_usec = db_logger->last.tv_nsec / 1000;
        if (this_usec == last_usec)
            this_usec++;
    }
    db_logger->last = smp->when;

    smp->values[0] = tstamp;
    tp = gmtime(&smp->when.tv_sec);
    smp->lengths[0] = snprintf(tstamp, sizeof(tstamp), "%04d-%02d-%02d %02d:%02d:%02d.%06u", tp->tm_year + 1900, tp->tm_mon + 1, tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec, (unsigned)this_usec);
    res = PQexecPrepared(db_logger->conn, smp->ptr.stmt, NUM_COLS, smp->values, smp->lengths, NULL, 0);
    if (res) {
        if (PQresultStatus(res) != PGRES_COMMAND_OK) {
            log_db_err(db_logger->conn, "unable to execute %s insert statment", smp->ptr.stmt);
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

static void *db_thread(void *ptr)
{
    db_logger_t *db_logger = ptr;
    sample_t *smp;

    if (db_setup(db_logger->conn) == PGRES_COMMAND_OK) {
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
    }
    PQfinish(db_logger->conn);
    return NULL;
}

static void enqueue(db_logger_t *db_logger, sample_t *smp, const char *stmt)
{
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

extern void db_logger_line(db_logger_t *db_logger, struct timespec *when, const char *line, const char *line_end)
{
    const char *ptr;
    sample_t *smp;

    if ((smp = malloc(sizeof(sample_t)))) {
        smp->when = *when;
        smp->ptr.data_ptr = smp->data;
        if ((ptr = parse_real(smp, 3, "<tmpr>", line, line_end))) {
            if ((ptr = parse_int(smp, 1, "<sensor>", ptr, line_end))) {
                if ((ptr = parse_int(smp, 2, "<id>", ptr, line_end))) {
                    if (parse_real(smp, 4, "<watts>", ptr, line_end)) {
                        enqueue(db_logger, smp, "power");
                        return;
                    }
                    else if (parse_int(smp, 4, "<imp>", ptr, line_end)) {
                        enqueue(db_logger, smp, "pulse");
                        return;
                    }
                }
            }
        }
        free(smp);
    }
    else
        log_syserr("unable to allocate sample");
}

extern db_logger_t *db_logger_new(const char *db_conn)
{
    db_logger_t *db_logger;
    int res;

    if ((db_logger = malloc(sizeof(db_logger_t)))) {
        if ((db_logger->conn = PQconnectdb(db_conn))) {
            db_logger->head = NULL;
            db_logger->tail = NULL;
            db_logger->last.tv_sec = 0;
            db_logger->last.tv_nsec = 0;
            if ((res = pthread_mutex_init(&db_logger->lock, NULL)) == 0)
                if ((res = pthread_cond_init(&db_logger->wait_data, NULL)) == 0)
                    if ((res = pthread_create(&db_logger->thread, NULL, db_thread, db_logger)) == 0)
                        return db_logger;
            log_msg("unable to create database thread - %s", strerror(res));
            PQfinish(db_logger->conn);
        }
        else
            log_syserr("unable to allocate datbase connection");
        free(db_logger);
    }
    else
        log_syserr("unable to allocate db-logger");
    return NULL;
}

extern void db_logger_free(db_logger_t *db_logger)
{
    sample_t smp;

    enqueue(db_logger, &smp, NULL);     // send "EOF" message.
    pthread_join(db_logger->thread, NULL);
    pthread_cond_destroy(&db_logger->wait_data);
    pthread_mutex_destroy(&db_logger->lock);
    free(db_logger);
}
