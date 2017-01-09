#include "cc-common.h"
#include "cgi-dbmain.h"
#include "log-db-err.h"

static const char db_conn[] =
    "dbname=currentcost user=cc_viewer password=aQuoxhzvCtEo";

int cgi_main(cgi_query_t *query) {
    int status = 2;
    PGconn *conn;
    
    if ((conn = PQconnectdb(db_conn))) {
	if (PQstatus(conn) == CONNECTION_OK)
	    status = cgi_db_main(query, conn);
	else
	    log_db_err(conn, "unable to connect to database");
	PQfinish(conn);
    } else
	log_syserr("unable to allocate database connection");
    return status;
}
