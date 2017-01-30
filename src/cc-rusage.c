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
    "      <th class=\"ru\">Resouce</th>"
    "      <th class=\"ru\">Used</th>"
    "    </tr>"
    "  </thead"
    "  <tbody>";

static const char rusage_html_tail[] =
    "  </tbody>"
    "</table>";

static const char cpu_fmt1[] =
    "<tr>"
    "  <td class=\"ru\">%s</td>"
    "  <td class=\"ru\">%ld.%06ld s</td>"
    "</tr>";

static const char cpu_fmt2[] =
    "<tr>"
    "  <td class=\"ru\">%s</td>"
    "  <td class=\"ru\">%'ld \u00B5s</td>"
    "</tr>";

static const char cnt_fmt[] =
    "<tr>"
    "  <td class=\"ru\">%s</td>"
    "  <td class=\"ru\">%'ld</td>"
    "</tr>";

static void send_cpu(FILE *cgi_str, const char *label, time_t secs, unsigned long usec) {
    if (secs > 0)
	fprintf(cgi_str, cpu_fmt1, label, secs, usec);
    else if (usec > 0)
	fprintf(cgi_str, cpu_fmt2, label, usec);
}

static void send_cnt(FILE *cgi_str, const char *label, unsigned long value) {
    if (value > 0)
	fprintf(cgi_str, cnt_fmt, label, value);
}

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
	send_cpu(cgi_str, "Elapsed Time", elapsed.tv_sec, elapsed.tv_nsec/1000);
	send_cpu(cgi_str, "User CPU", ru.ru_utime.tv_sec, ru.ru_utime.tv_usec);
	send_cpu(cgi_str, "System CPU", ru.ru_stime.tv_sec, ru.ru_stime.tv_usec);
	send_cnt(cgi_str, "Page reclaims", ru.ru_minflt);
	send_cnt(cgi_str, "Page faults", ru.ru_majflt);
	send_cnt(cgi_str, "Blocks in", ru.ru_inblock);
	send_cnt(cgi_str, "Blocks out", ru.ru_oublock);
	fwrite(rusage_html_tail, sizeof(rusage_html_tail)-1, 1, cgi_str);
    } else
	log_syserr("Unable to get resource usage");
}
