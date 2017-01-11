#ifndef CC_HTML_H
#define CC_HTML_H

#include <stdio.h>

extern const char default_dir[];
extern const char xml_file[];
extern const char date_iso[];
extern const char base_url[];
extern const char *sensor_names[];

extern void html_send_top(FILE *fp);
extern void html_send_tail(FILE *fp);

extern void html_esc(const char *data, FILE *fp);

#define html_puts(str, fp) { const char s[] = str; fwrite(s, sizeof(s)-1, 1, fp); }

#endif
