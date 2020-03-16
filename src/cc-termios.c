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

#define BUF_SIZE   128
#define INTERVAL   5

struct _cc_ctx {
    logger_t *logger;
    const char *db_conn;
    const char *port;
    struct termios tio;

};

const char prog_name[] = "cc-termios";

static const char log_file[] = "cc-termios.log";
static const char pid_file[] = "cc-termios.pid";
static const char default_port[] = "/dev/ttyUSB0";

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

static int main_loop(cc_ctx_t *ctx)
{
    int status, fd, init_port = 1;
    unsigned char buf[BUF_SIZE];
    ssize_t nbytes;

    if ((fd = open(ctx->port, O_RDONLY)) >= 0) {
        log_msg("initialisation complete, begin main loop");
        status = 0;
        while (!exit_requested) {
            if (init_port) {
                if (tcsetattr(fd, TCSANOW, &ctx->tio) == -1) {
                    log_syserr("unable to set terminal attributes on port '%s'", ctx->port);
                    status = 16;
                    break;
                }
                cb_count = init_port = 0;
            }
            if ((nbytes = read(fd, buf, sizeof buf)) > 0) {
                logger_data(ctx->logger, buf, nbytes);
                if (nbytes > 1)
                    cb_count = 0;
            }
            else {
                if (cb_count > 1) {
                    log_msg("port locked, re-initialising");
                    init_port = 1;
                }
            }
        }
        log_msg("shutting down on receipt of signal #%d", exit_requested);
        close(fd);
    }
    else {
        log_syserr("unable to open port '%s'", ctx->port);
        status = 10;
    }
    return status;
}

int cc_termios(cc_ctx_t * ctx)
{
    int status;
    struct sigaction sa;
    struct itimerval it;

    if ((ctx->logger = logger_new(ctx->db_conn))) {
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
                        ctx->tio.c_iflag = IGNBRK | IGNCR;
                        ctx->tio.c_oflag = 0;
                        ctx->tio.c_cflag = CS8 | CREAD | CLOCAL;
                        ctx->tio.c_lflag = 0;
                        cfsetospeed(&ctx->tio, B57600);
                        cfsetispeed(&ctx->tio, B57600);
                        ctx->tio.c_cc[VMIN] = 120;
                        ctx->tio.c_cc[VTIME] = 0;
                        status = main_loop(ctx);
                    }
                    else {
                        log_syserr("unable to set interval timer");
                        status = 14;
                    }
                }
                else {
                    log_syserr("unable to handler for SIGALRM");
                    status = 13;
                }
            }
            else {
                log_syserr("unable to set handler for SIGINT");
                status = 12;
            }
        }
        else {
            log_syserr("unable to set handler for SIGTERM");
            status = 11;
        }
        logger_free(ctx->logger);
    }
    else {
        log_syserr("unable to allocate logger");
        status = 8;
    }
    return status;
}

int main(int argc, char **argv)
{
    int status = 0;
    const char *dir = default_dir;
    cc_ctx_t ctx;
    int c;

    ctx.db_conn = NULL;
    ctx.port = default_port;

    while ((c = getopt(argc, argv, "d:D:p:")) != EOF) {
        switch (c) {
            case 'd':
                dir = optarg;
                break;
            case 'D':
                ctx.db_conn = optarg;
                break;
            case 'p':
                ctx.port = optarg;
                break;
            default:
                status = 1;
        }
    }
    if (status)
        fputs("Usage: cc-termios [ -d dir ] [ -D <db-conn> ] [ -p port ]\n", stderr);
    else
        status = cc_daemon(dir, log_file, pid_file, cc_termios, &ctx);
    return status;
}
