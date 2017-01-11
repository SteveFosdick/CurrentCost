#ifndef CC_RUSAGE
#define CC_RUSAGE

#include <stdio.h>
#include <time.h>

extern void cc_rusage(struct timespec *start, FILE *cgi_str);

#endif
