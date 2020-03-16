#include "cc-defs.h"
#include "cc-common.h"
#include "logger.h"
#include "file-logger.h"
#include "db-logger.h"

struct _logger_t {
    file_logger_t *file_logger;
    db_logger_t *db_logger;
    char *line_ptr;
    char line[MAX_LINE_LEN + 1];
};

extern logger_t *logger_new(const char *db_conn)
{
    logger_t *logger;

    if ((logger = malloc(sizeof(logger_t)))) {
        logger->line_ptr = logger->line;
        if ((logger->file_logger = file_logger_new())) {
            if (db_conn) {
                if ((logger->db_logger = db_logger_new(db_conn)))
                    return logger;
            }
            else {
                logger->db_logger = NULL;
                return logger;
            }
            file_logger_free(logger->file_logger);
        }
        free(logger);
    }
    return NULL;
}

extern void logger_free(logger_t * logger)
{
    file_logger_free(logger->file_logger);
    if (logger->db_logger)
        db_logger_free(logger->db_logger);
    free(logger);
}

static void invoke_loggers(logger_t *logger, char *end)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    file_logger_line(logger->file_logger, &tv, logger->line, end);
    if (logger->db_logger)
        db_logger_line(logger->db_logger, &tv, logger->line, end);
}

extern void logger_data(logger_t *logger, const unsigned char *data, size_t size)
{
    const unsigned char *src_ptr = data;
    const unsigned char *src_end = data + size;
    char *line_ptr = logger->line_ptr;
    char *line_max = logger->line + MAX_LINE_LEN - 1;
    int ch;

    while (src_ptr < src_end) {
        ch = *src_ptr++;
        if (ch >= 0x20 && ch <= 0x7e) {
            if (line_ptr < line_max)
                *line_ptr++ = ch;
            else {
                log_msg("warning: line too long");
                invoke_loggers(logger, line_ptr);
                line_ptr = logger->line;
            }
        }
        else if ((ch == '\n' || ch == '\r') && line_ptr > logger->line) {
            *line_ptr++ = '\n';
            invoke_loggers(logger, line_ptr);
            line_ptr = logger->line;
        }
    }
    logger->line_ptr = line_ptr;
}
