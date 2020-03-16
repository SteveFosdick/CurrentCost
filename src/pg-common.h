#ifndef CC_PG_COMMON
#define CC_PG_COMMON

#include <time.h>
#include <libpq-fe.h>

#define NUM_COLS  5
#define MAX_DATA  (NUM_COLS * 20)
#define TIME_STAMP_SIZE 32

typedef struct sample sample_t;

struct sample {
    sample_t *next;
    struct timespec when;
    union {
        const char *stmt;
        char *data_ptr;
    } ptr;
    int lengths[NUM_COLS];
    const char *values[NUM_COLS];
    char data[MAX_DATA];
};

extern const char power_sql[];
extern const char pulse_sql[];

extern void log_db_err(PGconn *conn, const char *msg, ...);
extern const char *parse_int(sample_t *smp, int index, const char *pat, const char *str, const char *end);
extern const char *parse_real(sample_t *smp, int index, const char *pat, const char *str, const char *end);

#endif
