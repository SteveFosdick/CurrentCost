#include "cc-common.h"
#include "daemon.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

static const char dev_null[] = "/dev/null";

int cc_daemon(const char *dir, const char *log_file, const char *pid_file,
	      daemon_cb callback, cc_ctx_t *ctx)
{
    int status = 0;
    int fd;
    FILE  *fp;
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
		if ((fp = fopen(pid_file, "w"))) {
		    if ((pid = fork()) == 0) {
			fclose(fp);
			if (setsid() >= 0) {
			    log_msg("child running callback");
			    status = callback(ctx);
			    log_msg("child callback returns %d", status);
			} else {
			    log_syserr("unable to set session id");
			    status = 7;
			}
		    } else {
			if (pid == -1) {
			    log_syserr("unable to fork");
			    status = 6;
			} else {
			    log_msg("parent, child_pid=%d", pid);
			    fprintf(fp, "%d\n", pid);
			    status = 0;
			}
			fclose(fp);
		    }
		} else {
		    log_syserr("unable to open pid file '%s' for writing");
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
