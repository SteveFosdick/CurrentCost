#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char **argv) {
    const char *arg;
    struct tm tm;

    while (--argc) {
	arg = *++argv;
	memset(&tm, 0, sizeof(struct tm));
	if (strptime(arg, "%Y-%m-%d %H:%M:%S", &tm))
	    printf("%s -> %ld\n", arg, mktime(&tm));
    }
}
