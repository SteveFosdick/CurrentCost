#ifndef FILE_LOGGER_INC
#define FILE_LOGGER_INC

#include <sys/time.h>

typedef struct _file_logger_t file_logger_t;

extern file_logger_t *file_logger_new();
extern void file_logger_free(file_logger_t *file_logger);

extern void file_logger_line(file_logger_t *file_logger, struct timeval *when,
			     const char *line, const char *end);

#endif
