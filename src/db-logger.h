#ifndef CC_DB_LOGGER
#define CC_DB_LOGGER

#include <sys/time.h>

typedef struct _db_logger_t db_logger_t;

extern db_logger_t *db_logger_new(const char *db_conn);
extern void db_logger_free(db_logger_t * logger);

extern void db_logger_line(db_logger_t *db_logger, struct timeval *when, const char *line, const char *end);
#endif
