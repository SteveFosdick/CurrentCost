#include "cgi-main.h"
#include "cc-rusage.h"
#include "cc-common.h"
#include <sys/resource.h>

static const char rusage_html_head[] =
    "<hr>"
    "<h2>Resource Usage</h2>"
    "<table>"
    "  <thead>"
    "    <tr>"
    "      <th>Resouce</th>"
    "      <th>Used</th>"
    "    </tr>"
    "  </thead"
    "  <tbody>";

static const char rusage_html_tail[] =
    "  </tbody>"
    "</table>";

static const char cpu_fmt[] =
    "<tr>"
    "  <td>%s</td>"
    "  <td>%ld.%06ld</td>"
    "</tr>";

static const char cnt_fmt[] =
    "<tr>"
    "  <td>%s</td>"
    "  <td>%'ld</td>"
    "</tr>";
    
void cc_rusage(struct timespec *start, FILE *cgi_str) {
    struct timespec end, elapsed;
    struct rusage ru;

    clock_gettime(CLOCK_REALTIME, &end);
    elapsed.tv_sec = end.tv_sec - start->tv_sec;
    elapsed.tv_nsec = end.tv_nsec - start->tv_nsec;
    if (elapsed.tv_nsec < 0) {
	elapsed.tv_nsec += 1000000000;
	elapsed.tv_sec--;
    }
    if (getrusage(RUSAGE_SELF, &ru) == 0) {
	fwrite(rusage_html_head, sizeof(rusage_html_head)-1, 1, cgi_str);
	fprintf(cgi_str, cpu_fmt, "Elapsed Time", elapsed.tv_sec, elapsed.tv_nsec/1000);
	fprintf(cgi_str, cpu_fmt, "User CPU", ru.ru_utime.tv_sec, ru.ru_utime.tv_usec);
	fprintf(cgi_str, cpu_fmt, "System CPU", ru.ru_stime.tv_sec, ru.ru_stime.tv_usec);
	fprintf(cgi_str, cnt_fmt, "Page reclaims", ru.ru_minflt);
	fprintf(cgi_str, cnt_fmt, "Page faults", ru.ru_majflt);
	fprintf(cgi_str, cnt_fmt, "Blocks in", ru.ru_inblock);
	fprintf(cgi_str, cnt_fmt, "Blocks out", ru.ru_oublock);
	fwrite(rusage_html_tail, sizeof(rusage_html_tail)-1, 1, cgi_str);
    } else
	log_syserr("Unable to get resource usage");
}
