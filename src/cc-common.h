#ifndef CC_COMMON_H
#define CC_COMMON_H

#include <stdarg.h>

#define MAX_SENSOR   10
#define ISO_DATE_LEN 21

extern const char prog_name[];
extern const char default_dir[];
extern const char xml_file[];
extern const char date_iso[];

extern void log_msg(const char *msg, ...);
extern void log_syserr(const char *msg, ...);

#endif
