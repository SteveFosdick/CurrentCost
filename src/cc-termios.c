#include "cc-common.h"
#include "logger.h"

#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <termios.h>
#include <unistd.h>

#define BUF_SIZE   100
#define INTERVAL   4

const char prog_name[] = "cc-termios";

static const char error_log[]    = "cc-termios.err";
static const char default_port[] = "/dev/usb/ttyUSB0";


static int cb_count = 0;

static void timer_cb(int sig) {
    cb_count++;
}

static int main_loop(const char *port, int ifd, struct termios *tio) {
    logger_t *logger;
    int init_port = 1;
    unsigned char buf[BUF_SIZE];
    ssize_t nbytes;

    if ((logger = logger_new())) {
	for (;;) {
	    if (init_port) {
		if (tcsetattr(ifd, TCSANOW, tio) == -1) {
		    log_syserr("unable to set terminal attributes on port '%s'", port);
		    return 5;
		}
		cb_count = init_port = 0;
	    }
	    if ((nbytes = read(ifd, buf, sizeof buf)) > 0) {
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
    } else {
	log_syserr("unable to allocate logger");
	return 4;
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
		    log_syserr("unable to set interval timer");
		    status = 6;
		}
	    } else {
		log_syserr("unable to set signal handler");
		status = 5;
	    }
	} else {
	    log_syserr("unable to open error log '%s'", error_log);
	    status = 4;
	}
    } else {
	log_syserr("unable to set session id");
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
		log_syserr("unable to fork");
		status = 2;
	    }
	}
	close(fd);
    } else {
	log_syserr("unable to open port '%s'", port);
	status = 1;
    }
    return status;
}
    
    
