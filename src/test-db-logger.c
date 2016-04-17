#include "cc-common.h"
#include "db-logger.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

const char prog_name[] = "test-db-testlogger";

static void prompt() {
    fputs("tdl> ", stdout);
    fflush(stdout);
}

int main(int argc, char **argv)
{
    db_logger_t *db_logger;
    int interactive = 0;
    struct timeval when;
    char line[MAX_LINE_LEN+1];

    if (argc != 2) {
	fputs("Usage: test-db-logger <db-conn-str>\n", stderr);
	return 1;
    }
    if ((db_logger = db_logger_new(argv[1])) == NULL) {
	fputs("test-db-logger: unable to create db logger\n", stderr);
	return 2;
    }
    if (isatty(0)) {
	interactive = 1;
	prompt();
    }
    while (fgets(line, sizeof(line), stdin)) {
	gettimeofday(&when, NULL);
	db_logger_line(db_logger, &when, line, line + strlen(line));
	sleep(1);
	if (interactive)
	    prompt();
    }
    db_logger_free(db_logger);
    return 0;
}
