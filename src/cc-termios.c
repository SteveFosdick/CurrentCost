#include "cc-common.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>

#define BUF_SIZE   100
#define INTERVAL   4
#define MSG_PREFIX "cc-termios: "

static const char error_log[]    = "cc-termios.err";
static const char default_port[] = "/dev/usb/ttyUSB0";
static const char date_iso8601[] = "%Y%m%dT%H%M%S";

static int cb_count = 0;

static void timer_cb(int sig) {
    cb_count++;
}

typedef enum {
    ST_GROUND,
    ST_GOT_LT,
    ST_GOT_M,
    ST_GOT_S,
    ST_GOT_G,
    ST_COPY
} state_t;

static void log_error(FILE *fp, const char *msg, ...) {
    time_t secs;
    struct tm *tp;
    char stamp[24];
    va_list ap;

    va_start(ap, msg);
    time(&secs);
    tp = gmtime(&secs);
    strftime(stamp, sizeof stamp, "%d/%m/%Y %H:%M:%S", tp);
    fprintf(stderr, "%s " MSG_PREFIX, stamp);
    vfprintf(stderr, msg, ap);
    putc('\n', stderr);
    if (fp) {
	strftime(stamp, sizeof stamp, date_iso8601, tp);
	fprintf(fp, "<error><host-tstamp>%s</host-tstamp><text>", stamp);
	vfprintf(fp, msg, ap);
	fputs("</text></error>\n", fp);
	fflush(fp);
    }
    va_end(ap);
}

static int main_loop(const char *port, int ifd, struct termios *tio) {
    state_t state = ST_GROUND;
    int old_day = 0;
    int init_port = 1;
    FILE *nfp, *ofp = NULL;
    time_t secs;
    struct tm *tp;
    char file[30], buf[BUF_SIZE], stamp[ISO_DATE_LEN];
    ssize_t nbytes;
    char *ptr, *end;
    int ch;

    for (;;) {
	if (init_port) {
	    if (tcsetattr(ifd, TCSANOW, tio) == -1) {
		log_error(ofp, "unable to set terminal attributes on port '%s' - %s", port, strerror(errno));
		return 4;
	    }
	    cb_count = init_port = 0;
	}
	if ((nbytes = read(ifd, buf, sizeof buf)) > 0) {
	    ptr = buf;
	    end = ptr + nbytes;
	    while (ptr < end) {
		ch = *ptr++;
		switch(state) {
		case ST_GROUND:
		    if (ch == '<')
			state = ST_GOT_LT;
		    break;
		case ST_GOT_LT:
		    if (ch == 'm')
			state = ST_GOT_M;
		    else
			state = ST_GROUND;
		    break;
		case ST_GOT_M:
		    if (ch == 's')
			state = ST_GOT_S;
		    else
			state = ST_GROUND;
		    break;
		case ST_GOT_S:
		    if (ch == 'g')
			state = ST_GOT_G;
		    else
			state = ST_GROUND;
		    break;
		case ST_GOT_G:
		    if (ch == '>') {
			time(&secs);
			tp = gmtime(&secs);
			if (tp->tm_mday != old_day) {
			    strftime(file, sizeof file, "cclog-%Y-%m-%d.xml", tp);
			    if ((nfp = fopen(file, "a"))) {
				if (ofp != NULL)
				    fclose(ofp);
				ofp = nfp;
				old_day = tp->tm_mday;
			    } else
				log_error(ofp, "unable to open file '%s' for append - %s", file, strerror(errno));
			}
			if (ofp != NULL) {
			    strftime(stamp, sizeof stamp, date_iso8601, tp);
			    fprintf(ofp, "<msg><host-tstamp>%s</host-tstamp>", stamp);
			}
			state = ST_COPY;
		    } else
			state = ST_GROUND;
		    break;
		case ST_COPY:
		    if (ofp != NULL)
			if ((ch >= 0x20 && ch <= 0x7e) || ch == '\n')
			    putc(ch, ofp);
		    if (ch == '\n')
			state = ST_GROUND;
		}
	    }
	    if (nbytes > 1)
		cb_count = 0;
	} else {
	    if (cb_count > 1) {
		log_error(ofp, "port locked, re-initialising");
		init_port = 1;
	    }
	    if (ofp != NULL)
		fflush(ofp);
	}
    }
}
    
static int run_child(const char *port, int ifd) {
    int status, fd;
    struct sigaction sa;
    struct itimerval it;
    struct termios   tio;

    if (setsid() >= 0) {
	if ((fd = open(error_log, O_WRONLY|O_CREAT|O_APPEND, 0666)) >= 0) {
	    if (fd != 2)
		dup2(fd, 2);
	    memset(&sa, 0, sizeof(sa));
	    sa.sa_handler = timer_cb;
	    if (sigaction(SIGALRM, &sa, NULL) == 0) {
		it.it_interval.tv_sec = INTERVAL;
		it.it_interval.tv_usec = 0;
		it.it_value.tv_sec = INTERVAL;
		it.it_value.tv_usec = 0;
		if (setitimer(ITIMER_REAL, &it, NULL) == 0) {
		    tio.c_iflag = IGNBRK|IGNCR;
		    tio.c_oflag = 0;
		    tio.c_cflag = CS8|CREAD|CLOCAL;
		    tio.c_lflag = 0;
		    cfsetospeed(&tio, B57600);
		    cfsetispeed(&tio, B57600);
		    tio.c_cc[VMIN] = 1;
		    tio.c_cc[VTIME] = 0;
		    
		    status = main_loop(port, ifd, &tio);
		} else {
		    fprintf(stderr, MSG_PREFIX "unable to set interval timer - %s\n", strerror(errno));
		    status = 6;
		}
	    } else {
		fprintf(stderr, MSG_PREFIX "unable to set signal handler - %s\n", strerror(errno));
		status = 5;
	    }
	} else {
	    fprintf(stderr, MSG_PREFIX "unable to open error log '%s' - %s\n", error_log, strerror(errno));
	    status = 4;
	}
    } else {
	fprintf(stderr, MSG_PREFIX "unable to set session id - %s\n", strerror(errno));
	status = 3;
    }
    return status;
}

int main(int argc, char **argv) {
    const char *port = default_port;
    int status, fd;
    pid_t pid;

    if (argc >= 2) {
	port = argv[1];
    }
    close(0);
    if ((fd = open(port, O_RDONLY)) >= 0) {
	status = 0;
	if ((pid = fork()) == 0) {
	    /* child process */
	    status = run_child(port, fd);
	} else {
	    if (pid == -1) {
		fprintf(stderr, MSG_PREFIX "unable to fork - %s\n", strerror(errno));
		status = 2;
	    }
	}
	close(fd);
    } else {
	fprintf(stderr, MSG_PREFIX "unable to open port '%s' - %s\n", port, strerror(errno));
	status = 1;
    }
    return status;
}
    
    
