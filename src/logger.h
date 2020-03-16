#ifndef LOGGER_INC
#define LOGGER_INC

#include <stdlib.h>

typedef struct _logger_t logger_t;

extern logger_t *logger_new(const char *db_conn);
extern void logger_free(logger_t * logger);

extern void logger_data(logger_t * logger,
                        const unsigned char *data, size_t size);

#endif
