#include "cc-common.h"
#include "daemon.h"
#include "logger.h"

#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>

#define BUF_SIZE   100
#define INTERVAL   4

typedef struct {
    const char *db_conn;
    const char *port;
} cc_opts_t;

const char prog_name[] = "cc-termios";

static const char log_file[] = "cc-termios.log";
static const char default_port[] = "/dev/usb/ttyUSB0";

static volatile int exit_requested = 0;
static volatile int cb_count = 0;

static void exit_handler(int sig)
{
    exit_requested = sig;
}

static void timer_cb(int sig)
{
    cb_count++;
}

static int main_loop(logger_t *logger, const char *port, int fd, struct termios *tio)
{
    int init_port = 1;
    unsigned char buf[BUF_SIZE];
    ssize_t nbytes;

    log_msg("initialisation complete, begin main loop");
    while (!exit_requested) {
	if (init_port) {
	    if (tcsetattr(fd, TCSANOW, tio) == -1) {
		log_syserr
		    ("unable to set terminal attributes on port '%s'",
		     port);
		return 16;
	    }
	    cb_count = init_port = 0;
	}
	if ((nbytes = read(fd, buf, sizeof buf)) > 0) {
	    logger_data(logger, buf, nbytes);
	    if (nbytes > 1)
		cb_count = 0;
	} else {
	    if (cb_count > 1) {
		log_msg("port locked, re-initialising");
		init_port = 1;
	    }
	}
    }
    log_msg("shutting down on receipt of signal #%d", exit_requested);
    return 0;
}

int child_process(void *user_data)
{
    cc_opts_t *opts = user_data;
    int status, fd;
    struct sigaction sa;
    struct itimerval it;
    struct termios tio;
    logger_t *logger;

    if ((fd = open(opts->port, O_RDONLY | O_NONBLOCK)) >= 0) {
        memset(&sa, 0, sizeof sa);
        sa.sa_handler = exit_handler;
        if (sigaction(SIGTERM, &sa, NULL) == 0) {
            if (sigaction(SIGINT, &sa, NULL) == 0) {
                sa.sa_handler = timer_cb;
                if (sigaction(SIGALRM, &sa, NULL) == 0) {
                    it.it_interval.tv_sec = INTERVAL;
                    it.it_interval.tv_usec = 0;
                    it.it_value.tv_sec = INTERVAL;
                    it.it_value.tv_usec = 0;
                    if (setitimer(ITIMER_REAL, &it, NULL) == 0) {
                        tio.c_iflag = IGNBRK | IGNCR;
                        tio.c_oflag = 0;
                        tio.c_cflag = CS8 | CREAD | CLOCAL;
                        tio.c_lflag = 0;
                        cfsetospeed(&tio, B57600);
                        cfsetispeed(&tio, B57600);
                        tio.c_cc[VMIN] = 1;
                        tio.c_cc[VTIME] = 0;

			if ((logger = logger_new(opts->db_conn))) {
			    status = main_loop(logger, opts->port, fd, &tio);
			    logger_free(logger);
			} else {
			    log_syserr("unable to allocate logger");
			    status = 13;
			}
                    } else {
                        log_syserr("unable to set interval timer");
                        status = 14;
                    }
                } else {
                    log_syserr("unable to handler for SIGALRM");
                    status = 13;
                }
            } else {
                log_syserr("unable to set handler for SIGINT");
                status = 12;
            }
        } else {
            log_syserr("unable to set handler for SIGTERM");
            status = 11;
        }
    } else {
        log_syserr("unable to open port '%s'", opts->port);
        status = 8;
    }
    return status;
}

int main(int argc, char **argv)
{
    int status = 0;
    const char *dir = default_dir;
    cc_opts_t cc_opts;
    int c;

    cc_opts.db_conn = NULL;
    cc_opts.port = default_port;

    while ((c = getopt(argc, argv, "d:D:p:")) != EOF) {
        switch (c) {
        case 'd':
            dir = optarg;
            break;
	case 'D':
	    cc_opts.db_conn = optarg;
	    break;
        case 'p':
            cc_opts.port = optarg;
            break;
        default:
            status = 1;
        }
    }
    if (status)
        fputs("Usage: cc-termios [ -d dir ] [ -D <db-conn> ] [ -p port ]\n", stderr);
    else
        status = cc_daemon(dir, log_file, child_process, &cc_opts);
    return status;
}
