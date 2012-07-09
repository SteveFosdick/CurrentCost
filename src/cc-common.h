#ifndef CC_COMMON_H
#define CC_COMMON_H

#include <stdarg.h>

extern const char prog_name[];
extern void log_msg(const char *msg, ...);
extern void log_syserr(const char *msg, ...);

#endif
