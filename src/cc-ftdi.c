#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include <ftdi.h>

static const char dflt_dir[] = "/share/fozzy/Data/CurrentCost";
static const char log_file[] = "cc-ftdi.log";
static const char dev_null[] = "/dev/null";
static const char xml_file[] = "cc-%Y-%m-%d.xml"; 
static const char date_iso[] = "%Y%m%dT%H%M%SZ";

#define DEFAULT_VENDOR_ID  0x0403
#define DEFAULT_PRODUCT_ID 0x6001
#define DEFAULT_INTERFACE  0
#define MSG_PREFIX "cc-ftdi: "

static volatile int exit_requested = 0;

static void exit_handler(int sig) {
    exit_requested = sig;
}

static void log_msg(const char *msg, ...) {
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
}

static void report_ftdi_err(int result, struct ftdi_context *ftdi, const char *msg) {
    log_msg("%s: %d (%s)", msg, result, ftdi_get_error_string(ftdi));
}

typedef enum {
    ST_GROUND,
    ST_GOT_LT,
    ST_GOT_M,
    ST_GOT_S,
    ST_GOT_G,
    ST_COPY
} state_t;

static int main_loop(struct ftdi_context *ftdi) {
    state_t state = ST_GROUND;
    unsigned char buf[4096];
    unsigned char *ptr, *end, *copy = NULL;
    int status = 0;
    int old_day = 0;
    int result, ch, flush;
    FILE *nfp, *ofp = NULL;
    time_t secs;
    struct tm *tp;
    char file[30], stamp[16];

    log_msg("initialisation complete, begin main loop");
    while (!exit_requested) {
	result = ftdi_read_data(ftdi, buf, sizeof buf);
	if (result > 0) {
	    flush = 0;
	    ptr = buf;
	    end = ptr + result;
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
			    strftime(file, sizeof file, xml_file, tp);
			    if ((nfp = fopen(file, "a"))) {
				if (ofp != NULL)
				    fclose(ofp);
				ofp = nfp;
				old_day = tp->tm_mday;
			    } else
				log_msg("unable to open file '%s' for append - %s", file, strerror(errno));
			}
			if (ofp != NULL) {
			    strftime(stamp, sizeof stamp, date_iso, tp);
			    fprintf(ofp, "<msg><host-tstamp>%s</host-tstamp>", stamp);
			}
			state = ST_COPY;
			copy = ptr;
		    } else
			state = ST_GROUND;
		    break;
		case ST_COPY:
		    if (ch == '\n') {
			if (ofp != NULL)
			    fwrite(copy, ptr-copy, 1, ofp);
			state = ST_GROUND;
			copy = NULL;
			flush = 1;
		    }
		}
	    }
	    if (copy) {
		if (ofp != NULL)
		    fwrite(copy, ptr-copy, 1, ofp);
		copy = NULL;
	    }
	    if (flush && ofp != NULL)
		fflush(ofp);
	} else if (result < 0) {
	    report_ftdi_err(result, ftdi, "read error");
	    status = 13;
	    sleep(1);
	}
    }
    log_msg("shutting down on receipt of signal #%d", exit_requested);
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
			    log_msg("unable to set handler for SIGINT - %s", strerror(errno));
			    status = 12;
			}
		    } else {
			log_msg("unable to set handler for SIGTERM - %s", strerror(errno));
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
			log_msg("unable to set session id - %s", strerror(errno));
			status = 6;
		    }
		}
		else if (pid == -1) {
		    log_msg("unable to fork - %s", strerror(errno));
		    status = 5;
		}
	    } else {
		fprintf(stderr, MSG_PREFIX "unable to open log file '%s' - %s\n", log_file, strerror(errno));
		status = 4;
	    }
	} else {
	    fprintf(stderr, MSG_PREFIX "unable to chdir to '%s' - %s\n", dir, strerror(errno));
	    status = 3;
	}
    } else {
	fprintf(stderr, MSG_PREFIX "unable to open NULL device '%s' - %s\n", dev_null, strerror(errno));
	status = 2;
    }
    return status;
}

int main(int argc, char **argv) {
    int status      = 0;
    int vendor_id   = DEFAULT_VENDOR_ID;
    int product_id  = DEFAULT_PRODUCT_ID;
    int interface   = DEFAULT_INTERFACE;
    const char *dir = dflt_dir;
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
