#include "cc-common.h"
#include "daemon.h"
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

static int main_loop(const char *port, int fd, struct termios *tio)
{
    logger_t *logger;
    int init_port = 1;
    unsigned char buf[BUF_SIZE];
    ssize_t nbytes;

    if ((logger = logger_new())) {
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
        return 0;
    } else {
        log_syserr("unable to allocate logger");
        return 15;
    }
}

int child_process(void *user_data)
{
    const char *port = user_data;
    int status, fd;
    struct sigaction sa;
    struct itimerval it;
    struct termios tio;

    if ((fd = open(port, O_RDONLY | O_NONBLOCK)) >= 0) {
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

                        status = main_loop(port, fd, &tio);
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
        log_syserr("unable to open port '%s'", port);
        status = 8;
    }
    return status;
}

int main(int argc, char **argv)
{
    int status = 0;
    const char *dir = default_dir;
    const char *port = default_port;
    int c;

    while ((c = getopt(argc, argv, "d:p:")) != EOF) {
        switch (c) {
        case 'd':
            dir = optarg;
            break;
        case 'p':
            port = optarg;
            break;
        default:
            status = 1;
        }
    }
    if (status)
        fputs("Usage: cc-termios [ -d dir ] [ -p port ]\n", stderr);
    else
        status = cc_daemon(dir, log_file, child_process, (void *) port);
    return status;
}
