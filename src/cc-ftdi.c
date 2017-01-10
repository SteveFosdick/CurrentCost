#include "cc-common.h"
#include "daemon.h"
#include "logger.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sched.h>
#include <unistd.h>

#include <ftdi.h>

struct _cc_ctx {
    logger_t   *logger;
    const char *db_conn;
    int        vendor_id;
    int        product_id;
    int        interface;
    struct ftdi_context ftdi;
} cc_opts_t;

const char prog_name[] = "cc-ftdi";

static const char log_file[] = "cc-ftdi.log";
static const char pid_file[] = "cc-ftdi.pid";

#define DEFAULT_VENDOR_ID  0x0403
#define DEFAULT_PRODUCT_ID 0x6001
#define DEFAULT_INTERFACE  1
#define MSG_PREFIX "cc-ftdi: "

static volatile int exit_requested = 0;

static void exit_handler(int sig) {
    exit_requested = sig;
}

static void report_ftdi_err(int result, cc_ctx_t *ctx, const char *msg) {
    log_msg("%s: %d (%s)", msg, result, ftdi_get_error_string(&ctx->ftdi));
}

static int main_loop(cc_ctx_t *ctx) {
    unsigned char buf[4096];
    int status = 0;
    int result;

    log_msg("initialisation complete, begin main loop");
    while (!exit_requested) {
	result = ftdi_read_data(&ctx->ftdi, buf, sizeof buf);
	if (result > 0)
	    logger_data(ctx->logger, buf, result);
	else {
	    if (result < 0) {
		report_ftdi_err(result, ctx, "read error");
		status = 14;
	    }
	    sleep(1);
	}
    }
    log_msg("shutting down on receipt of signal #%d", exit_requested);
    return status;
}

static int ftdi_main(cc_ctx_t *ctx)
{
    int status, res;
    struct sched_param sp;

    if ((res = ftdi_init(&ctx->ftdi)) == 0) {
        ftdi_set_interface(&ctx->ftdi, ctx->interface);
        res = ftdi_usb_open(&ctx->ftdi, ctx->vendor_id, ctx->product_id);
        if (res == 0) {
            if ((res = ftdi_set_baudrate(&ctx->ftdi, 57600)) == 0) {
                res = ftdi_set_line_property(&ctx->ftdi,8,STOP_BIT_1,NONE);
                if (res == 0) {
		    sp.sched_priority = 50;
		    if (sched_setscheduler(0, SCHED_FIFO, &sp) < 0)
			log_syserr("unable to set real-time priority");
		    status = main_loop(ctx);
                } else {
                    report_ftdi_err(res, ctx, "unable to set line properties");
                    status = 10;
                }
            } else {
                report_ftdi_err(res, ctx, "unable to set baud rate");
                status = 9;
            }
        } else {
            report_ftdi_err(res, ctx, "unable to open FTDT device");
            status = 8;
        }
        ftdi_deinit(&ctx->ftdi);
    } else {
        report_ftdi_err(res, ctx, "unable to initialise FTDI context");
        status = 7;
    }
    return status;
}

int cc_ftdi(cc_ctx_t *ctx) {
    int status;
    struct sigaction sa;
    
    if ((ctx->logger = logger_new(ctx->db_conn))) {
	memset(&sa, 0, sizeof sa);
	sa.sa_handler = exit_handler;
	if (sigaction(SIGTERM, &sa, NULL) == 0) {
	    if (sigaction(SIGINT, &sa, NULL) == 0)
		status = ftdi_main(ctx);
	    else {
		log_syserr("unable to set handler for SIGINT");
		status = 12;
	    }
	} else {
	    log_syserr("unable to set handler for SIGTERM");
	    status = 11;
	}
	logger_free(ctx->logger);
    } else {
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
    ctx.vendor_id = DEFAULT_VENDOR_ID;
    ctx.product_id = DEFAULT_PRODUCT_ID;
    ctx.interface = DEFAULT_INTERFACE;

    while ((c = getopt(argc, argv, "d:D:i:p:v:")) != EOF) {
        switch (c) {
        case 'd':
            dir = optarg;
            break;
	case 'D':
	    ctx.db_conn = optarg;
	    break;
        case 'i':
            ctx.interface = strtoul(optarg, NULL, 0);
            break;
        case 'p':
            ctx.product_id = strtoul(optarg, NULL, 0);
            break;
        case 'v':
            ctx.vendor_id = strtoul(optarg, NULL, 0);
            break;
        default:
            status = 1;
        }
    }
    if (status)
        fputs
            ("Usage: cc-ftdi [ -d dir ] [ -D <db-conn> ] [ -i interface ] [ -p product-id ] [ -v vendor-id ]\n",
             stderr);
    else
        status = cc_daemon(dir, log_file, pid_file, cc_ftdi, &ctx);
    return status;
}
