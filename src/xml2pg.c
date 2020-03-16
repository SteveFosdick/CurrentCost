#include "cc-defs.h"
#include "cc-common.h"
#include "pg-common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char prog_name[] = "xml2pg";

static void insert(PGconn *conn, sample_t *smp, const char *stmt)
{
    char tstamp[TIME_STAMP_SIZE];
    struct tm *tp = gmtime(&smp->when.tv_sec);
    smp->lengths[0] = snprintf(tstamp, sizeof(tstamp), "%04d-%02d-%02d %02d:%02d:%02d.%06u", tp->tm_year + 1900, tp->tm_mon + 1, tp->tm_mday, tp->tm_hour, tp->tm_min, tp->tm_sec, (unsigned)smp->when.tv_nsec);
    smp->values[0] = tstamp;
    PGresult *res = PQexecPrepared(conn, stmt, NUM_COLS, smp->values, smp->lengths, NULL, 0);
    if (res) {
        if (PQresultStatus(res) != PGRES_COMMAND_OK)
            log_db_err(conn, "unable to execute %s insert statment", stmt);
        PQclear(res);
}
    else
        log_syserr("out of memory executing %s SQL", stmt);
}

static void xml2pg(PGconn *conn, FILE *in)
{
    char line[MAX_LINE_LEN];
    time_t this_secs, last_secs;
    unsigned this_usecs, last_usecs;

    last_secs = last_usecs = 0;
    while (fgets(line, sizeof(line), in)) {
        sample_t smp;
        smp.ptr.data_ptr = smp.data;
        const char *line_end = strchr(line, '\n');
        if (!line_end) {
            log_msg("line too long");
            line_end = line + strlen(line);
        }
        const char *ptr = parse_real(&smp, 0, "<host-tstamp>", line, line_end);
        if (ptr) {
            char *tsend;
            this_secs = strtoul(smp.values[0], &tsend, 10);
            this_usecs = strtoul(tsend, NULL, 10);
            if (this_secs < last_secs || (this_secs == last_secs && this_usecs <= last_usecs)) {
                this_secs = last_secs;
                this_usecs = ++last_usecs;
            }
            else {
                last_secs = this_secs;
                last_usecs = this_usecs;
            }
            if ((ptr = parse_real(&smp, 3, "<tmpr>", ptr, line_end))) {
                if ((ptr = parse_int(&smp, 1, "<sensor>", ptr, line_end))) {
                    if ((ptr = parse_int(&smp, 2, "<id>", ptr, line_end))) {
                        smp.when.tv_sec = this_secs;
                        smp.when.tv_nsec = this_usecs;
                        if (parse_real(&smp, 4, "<watts>", ptr, line_end))
                            insert(conn, &smp, "power");
                        else if (parse_int(&smp, 4, "<imp>", ptr, line_end))
                            insert(conn, &smp, "pulse");
                    }
                }
            }
        }
    }
}

int main(int argc, char **argv)
{
    int status;
    if (--argc == 0) {
        fputs("Usage: xml2pg <db-conn> [ <xml-file> ...]\n", stderr);
        status = 1;
    }
    else {
        const char *db = *++argv;
        PGconn *conn = PQconnectdb(db);
        if (PQstatus(conn) != CONNECTION_OK) {
            log_db_err(conn, "unable to connect to database '%s'", db);
            status = 2;
        }
        else {
            PGresult *res;
            if ((res = PQexec(conn, "SET TIME ZONE UTC"))) {
                if (PQresultStatus(res) == PGRES_COMMAND_OK) {
                    PQclear(res);
                    if ((res = PQprepare(conn, "power", power_sql, 0, NULL))) {
                        if (PQresultStatus(res) == PGRES_COMMAND_OK) {
                            PQclear(res);
                            if ((res = PQprepare(conn, "pulse", pulse_sql, 0, NULL))) {
                                if (PQresultStatus(res) == PGRES_COMMAND_OK) {
                                    PQclear(res);
                                    status = 0;
                                    if (argc == 1)
                                        xml2pg(conn, stdin);
                                    else {
                                        while (--argc) {
                                            const char *file = *++argv;
                                            FILE *in = fopen(file, "r");
                                            if (in) {
                                                fprintf(stderr, "%s\n", file);
                                                xml2pg(conn, in);
                                                fclose(in);
                                            }
                                            else {
                                                log_syserr("unable to open '%s' for reading", file);
                                                status = 3;
                                            }
                                        }
                                    }
                                }
                                else {
                                    log_db_err(conn, "error preparing pulse SQL");
                                    PQclear(res);
                                    status = 2;
                                }
                            }
                            else {
                                log_syserr("out of memory preparing pulse SQL");
                                status = 2;
                            }
                        }
                        else {
                            log_db_err(conn, "error preparing power SQL");
                            PQclear(res);
                            status = 2;
                        }
                    }
                    else {
                        log_syserr("out of memory preparing power SQL");
                        status = 2;
                    }
                }
                else {
                    log_db_err(conn, "error setting timezone");
                    PQclear(res);
                    status = 2;
                }
            }
            else {
                log_syserr("out of memory preparing time zone SQL");
                status = 2;
            }
        }
    }
    return status;
}
