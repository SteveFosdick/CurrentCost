#include "cc-common.h"
#include "logger.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include <ftdi.h>

const char prog_name[] = "cc-ftdi";

static const char log_file[] = "cc-ftdi.log";
static const char dev_null[] = "/dev/null";

#define DEFAULT_VENDOR_ID  0x0403
#define DEFAULT_PRODUCT_ID 0x6001
#define DEFAULT_INTERFACE  0
#define MSG_PREFIX "cc-ftdi: "

static volatile int exit_requested = 0;

static void exit_handler(int sig) {
    exit_requested = sig;
}

static void report_ftdi_err(int result, struct ftdi_context *ftdi, const char *msg) {
    log_msg("%s: %d (%s)", msg, result, ftdi_get_error_string(ftdi));
}

static int main_loop(struct ftdi_context *ftdi) {
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

static int cc_ftdi(int vendor_id, int product_id, int interface) {
    int status, result;
    struct ftdi_context ftdi;
    struct sigaction sa;

    if ((result = ftdi_init(&ftdi)) == 0) {
	ftdi_set_interface(&ftdi, 1);
	if ((result = ftdi_usb_open(&ftdi, 0x0403, 0x6001)) == 0) {
	    if ((result = ftdi_set_baudrate(&ftdi, 57600)) == 0) {
		if ((result = ftdi_set_line_property(&ftdi, 8, STOP_BIT_1, NONE)) == 0) {
		    memset(&sa, 0, sizeof sa);
		    sa.sa_handler = exit_handler;
		    if (sigaction(SIGTERM, &sa, NULL) == 0) {
			if (sigaction(SIGINT, &sa, NULL) == 0)
			    status = main_loop(&ftdi);
			else {
			    log_syserr("unable to set handler for SIGINT");
			    status = 12;
			}
		    } else {
			log_syserr("unable to set handler for SIGTERM");
			status = 11;
		    }
		} else {
		    report_ftdi_err(result, &ftdi, "unable to set line properties");
		    status = 10;
		}
	    }
	    else {
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

static int start_daemon(const char *dir, int vendor_id, int product_id, int interface) {
    int status = 0;
    int  fd;
    pid_t pid;

    if ((fd = open(dev_null, O_RDWR)) >= 0) {
	if (fd != 0)
	    dup2(fd, 0);
	if (fd != 1)
	    dup2(fd, 1);
	if (fd > 1)
	    close(fd);
	if (chdir(dir) == 0) {
	    if ((fd = open(log_file, O_WRONLY|O_APPEND|O_CREAT, 0644)) >= 0) {
		if (fd != 2) {
		    dup2(fd, 2);
		    close(fd);
		}
		if ((pid = fork()) == 0) {
		    if (setsid() >= 0)
			status = cc_ftdi(vendor_id, product_id, interface);
		    else {
			log_syserr("unable to set session id");
			status = 6;
		    }
		}
		else if (pid == -1) {
		    log_syserr("unable to fork");
		    status = 5;
		}
	    } else {
		log_syserr("unable to open log file '%s'", log_file);
		status = 4;
	    }
	} else {
	    log_syserr("unable to chdir to '%s'", dir);
	    status = 3;
	}
    } else {
	log_syserr("unable to open NULL device '%s'", dev_null);
	status = 2;
    }
    return status;
}

int main(int argc, char **argv) {
    int status      = 0;
    int vendor_id   = DEFAULT_VENDOR_ID;
    int product_id  = DEFAULT_PRODUCT_ID;
    int interface   = DEFAULT_INTERFACE;
    const char *dir = default_dir;
    int c;

    while ((c = getopt(argc, argv, "d:i:p:v:")) != EOF) {
	switch(c) {
	case 'd':
	    dir = optarg;
	    break;
	case 'i':
	    interface = strtoul(optarg, NULL, 0);
	    break;
	case 'p':
	    product_id = strtoul(optarg, NULL, 0);
	    break;
	case 'v':
	    vendor_id = strtoul(optarg, NULL, 0);
	    break;
	default:
	    status = 1;
	}
    }
    if (status)
	fputs("Usage: cc-ftdi [ -d dir ] [ -i interface ] [ -p product-id ] [ -v vendor-id ]\n", stderr);
    else
	status = start_daemon(dir, vendor_id, product_id, interface);
    return status;
}
