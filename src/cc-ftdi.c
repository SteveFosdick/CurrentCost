#include "cc-common.h"
#include "daemon.h"
#include "logger.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <sched.h>

#include <ftdi.h>

typedef struct {
    int vendor_id;
    int product_id;
    int interface;
} cc_opts_t;

const char prog_name[] = "cc-ftdi";

static const char log_file[] = "cc-ftdi.log";
static const char dev_null[] = "/dev/null";

#define DEFAULT_VENDOR_ID  0x0403
#define DEFAULT_PRODUCT_ID 0x6001
#define DEFAULT_INTERFACE  1
#define MSG_PREFIX "cc-ftdi: "

static volatile int exit_requested = 0;

static void exit_handler(int sig)
{
    exit_requested = sig;
}

static void report_ftdi_err(int result,
                            struct ftdi_context *ftdi, const char *msg)
{
    log_msg("%s: %d (%s)", msg, result, ftdi_get_error_string(ftdi));
}

static int main_loop(struct ftdi_context *ftdi)
{
    logger_t *logger;
    unsigned char buf[4096];
    int status = 0;
    int result;

    if ((logger = logger_new())) {
        log_msg("initialisation complete, begin main loop");
        while (!exit_requested) {
            result = ftdi_read_data(ftdi, buf, sizeof buf);
            if (result > 0)
                logger_data(logger, buf, result);
            else {
                if (result < 0) {
                    report_ftdi_err(result, ftdi, "read error");
                    status = 14;
                }
                sleep(1);
            }
        }
        log_msg("shutting down on receipt of signal #%d", exit_requested);
    } else {
        log_syserr("unable to allocate logger");
        status = 13;
    }
    return status;
}

static int child_process(void *user_data)
{
    cc_opts_t *opts = user_data;
    int status, result;
    struct ftdi_context ftdi;
    struct sigaction sa;
    struct sched_param sp;

    if ((result = ftdi_init(&ftdi)) == 0) {
        ftdi_set_interface(&ftdi, opts->interface);
        result = ftdi_usb_open(&ftdi, opts->vendor_id, opts->product_id);
        if (result == 0) {
            if ((result = ftdi_set_baudrate(&ftdi, 57600)) == 0) {
                result = ftdi_set_line_property(&ftdi, 8, STOP_BIT_1, NONE);
                if (result == 0) {
                    memset(&sa, 0, sizeof sa);
                    sa.sa_handler = exit_handler;
                    if (sigaction(SIGTERM, &sa, NULL) == 0) {
                        if (sigaction(SIGINT, &sa, NULL) == 0) {
                            sp.sched_priority = 50;
                            if (sched_setscheduler(0, SCHED_FIFO, &sp) < 0)
                                log_syserr
                                    ("unable to set real-time priority");
                            status = main_loop(&ftdi);
                        } else {
                            log_syserr("unable to set handler for SIGINT");
                            status = 12;
                        }
                    } else {
                        log_syserr("unable to set handler for SIGTERM");
                        status = 11;
                    }
                } else {
                    report_ftdi_err(result, &ftdi,
                                    "unable to set line properties");
                    status = 10;
                }
            } else {
                report_ftdi_err(result, &ftdi, "unable to set baud rate");
                status = 9;
            }
        } else {
            report_ftdi_err(result, &ftdi, "unable to open FTDT device");
            status = 8;
        }
        ftdi_deinit(&ftdi);
    } else {
        report_ftdi_err(result, &ftdi, "unable to initialise FTDI context");
        status = 7;
    }
    return status;
}

int main(int argc, char **argv)
{
    int status = 0;
    const char *dir = default_dir;
    cc_opts_t cc_opts;
    int c;

    cc_opts.vendor_id = DEFAULT_VENDOR_ID;
    cc_opts.product_id = DEFAULT_PRODUCT_ID;
    cc_opts.interface = DEFAULT_INTERFACE;

    while ((c = getopt(argc, argv, "d:i:p:v:")) != EOF) {
        switch (c) {
        case 'd':
            dir = optarg;
            break;
        case 'i':
            cc_opts.interface = strtoul(optarg, NULL, 0);
            break;
        case 'p':
            cc_opts.product_id = strtoul(optarg, NULL, 0);
            break;
        case 'v':
            cc_opts.vendor_id = strtoul(optarg, NULL, 0);
            break;
        default:
            status = 1;
        }
    }
    if (status)
        fputs
            ("Usage: cc-ftdi [ -d dir ] [ -i interface ] [ -p product-id ] [ -v vendor-id ]\n",
             stderr);
    else
        status = cc_daemon(dir, log_file, child_process, &cc_opts);
    return status;
}
