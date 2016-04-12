#include "cc-common.h"
#include "parsefile.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sqlite3.h>

typedef struct _sqlite_ud {
    sqlite3      *db;
    sqlite3_stmt *sample_stmt;
    sqlite3_stmt *pulse_stmt;
} sqlite_ud_t;

const char prog_name[] = "xml2sqlite";
const char db_file[] = DEFAULT_DIR "/cc.db";
const char sample_sql[] = "INSERT INTO samples VALUES (?, ?, ?, ?)";
const char pulse_sql[] = "INSERT INTO pulses VALUES (?, ?, ?, ?, ?)";
const char begin_txn[] = "BEGIN TRANSACTION";
const char commit_txn[] = "COMMIT TRANSACTION";
const char rollback_txn[] = "ROLLBACK TRANSACTION";

static void log_sqlite3_err(const char *msg, sqlite_ud_t *ud) {
    log_msg("%s: %s (%d)", msg, sqlite3_errmsg(ud->db), sqlite3_errcode(ud->db));
}

static mf_status sample_cb(pf_context * ctx, pf_sample * smp)
{
    mf_status status;
    sqlite_ud_t *ud = ctx->user_data;
    sqlite3_stmt *stmt = ud->sample_stmt;
    int rc;
    if ((rc = sqlite3_bind_int64(stmt, 1, smp->timestamp)) == SQLITE_OK) {
	if ((rc = sqlite3_bind_double(stmt, 2, smp->temp)) == SQLITE_OK) {
	    if ((rc = sqlite3_bind_int(stmt, 3, smp->sensor)) == SQLITE_OK) {
		if ((rc = sqlite3_bind_double(stmt, 4, smp->data.watts)) == SQLITE_OK) {
		    if ((rc = sqlite3_step(stmt)) == SQLITE_DONE)
			status = MF_SUCCESS;
		    else {
			log_sqlite3_err("unable to prepare sample insert statement", ud);
			status = MF_FAIL;
		    }
		} else {
		    log_sqlite3_err("unable to bind watts to sample insert statement", ud);
		    status = MF_FAIL;
		}
	    } else {
		log_sqlite3_err("unable to bind sensor to sample insert statement", ud);
		status = MF_FAIL;
	    }
	} else {
	    log_sqlite3_err("unable to bind temperature to sample insert statement", ud);
	    status = MF_FAIL;
	}
    } else {
	log_sqlite3_err("unable to bind time stamp to sample insert statement", ud);
	status = MF_FAIL;
    }
    sqlite3_reset(stmt);
    return status;
}

static mf_status pulse_cb(pf_context * ctx, pf_sample * smp)
{
    mf_status status;
    sqlite_ud_t *ud = ctx->user_data;
    sqlite3_stmt *stmt = ud->pulse_stmt;
    int rc;
    if ((rc = sqlite3_bind_int64(stmt, 1, smp->timestamp)) == SQLITE_OK) {
	if ((rc = sqlite3_bind_double(stmt, 2, smp->temp)) == SQLITE_OK) {
	    if ((rc = sqlite3_bind_int(stmt, 3, smp->sensor)) == SQLITE_OK) {
		if ((rc = sqlite3_bind_int64(stmt, 4, smp->data.pulse.count)) == SQLITE_OK) {
		    if ((rc = sqlite3_bind_int(stmt, 5, smp->data.pulse.ipu)) == SQLITE_OK) {
			if ((rc = sqlite3_step(stmt)) == SQLITE_DONE)
			    status = MF_SUCCESS;
			else {
			    log_sqlite3_err("unable to execute pulse insert statement", ud);
			    status = MF_FAIL;
			}
		    } else {
			log_sqlite3_err("unable to bind ipu to pulse insert statement", ud);
			status = MF_FAIL;
		    }
		} else {
		    log_sqlite3_err("unable to bind count to pulse insert statement", ud);
		    status = MF_FAIL;
		}
	    } else {
		log_sqlite3_err("unable to bind sensor to pulse insert statement", ud);
		status = MF_FAIL;
	    }
	} else {
	    log_sqlite3_err("unable to bind temperature to pulse insert statement", ud);
	    status = MF_FAIL;
	}
    } else {
	log_sqlite3_err("unable to bind time stamp to pulse insert statement", ud);
	status = MF_FAIL;
    }
    sqlite3_reset(stmt);
    return status;
}

static int do_sql(sqlite3 *db, const char *sql, int len) {
    sqlite3_stmt *stmt;
    int rc;

    if ((rc = sqlite3_prepare_v2(db, sql, len, &stmt, NULL)) == SQLITE_OK)
	if ((rc = sqlite3_step(stmt)) == SQLITE_DONE)
	    rc = SQLITE_OK;
    sqlite3_finalize(stmt);
    return rc;
}

int main(int argc, char **argv)
{
    int status = 0, rc;
    sqlite_ud_t ud;
    pf_context *pf;
    const char *arg;

    if ((rc = sqlite3_open(db_file, &ud.db)) == 0) {
	if ((rc = sqlite3_prepare_v2(ud.db, sample_sql, sizeof(sample_sql)-1, &ud.sample_stmt, NULL)) == SQLITE_OK) {
	    if ((rc = sqlite3_prepare_v2(ud.db, pulse_sql, sizeof(pulse_sql)-1, &ud.pulse_stmt, NULL)) == SQLITE_OK) {
		if ((rc = do_sql(ud.db, begin_txn, sizeof(begin_txn)-1)) == SQLITE_OK) {
		    if ((pf = pf_new())) {
			pf->sample_cb = sample_cb;
			pf->pulse_cb = pulse_cb;
			pf->user_data = &ud;
			while (--argc) {
			    arg = *++argv;
			    if (arg[0] == '-' && arg[1] == 'f')
				pf->file_cb = tf_parse_cb_forward;
			    else if (arg[0] == '-' && arg[1] == 'b')
				pf->file_cb = tf_parse_cb_backward;
			    else if (pf_parse_file(pf, arg) == MF_FAIL) {
				status = 4;
				break;
			    }
			}
			if (status == 0) {
			    if ((rc = do_sql(ud.db, commit_txn, sizeof(commit_txn)-1)) != 0)
				log_sqlite3_err("unable to commit transaction", &ud);
			} else {
			    if ((rc = do_sql(ud.db, rollback_txn, sizeof(rollback_txn)-1)) != 0)
				log_sqlite3_err("unable to rollback transaction", &ud);
			}
		    } else
			status = 3;
		} else {
		    log_sqlite3_err("unable to start transaction", &ud);
		    status = 2;
		}
		sqlite3_finalize(ud.pulse_stmt);
	    } else {
		log_sqlite3_err("unable to prepare pulse insert statement", &ud);
		status = 2;
	    }
	    sqlite3_finalize(ud.sample_stmt);
	} else {
	    log_sqlite3_err("unable to prepare sample insert statement", &ud);
	    status = 2;
	}
	sqlite3_close(ud.db);
    } else {
	log_sqlite3_err("unable to open sqlite database", &ud);
        status = 1;
    }
    return status;
}
