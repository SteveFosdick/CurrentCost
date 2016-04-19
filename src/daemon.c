#include "cc-common.h"
#include "daemon.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

static const char dev_null[] = "/dev/null";

int cc_daemon_fork(const char *pid_file, daemon_cb callback, void *user_data) {
    int   status;
    FILE  *fp;
    pid_t pid;

    if ((fp = fopen(pid_file, "w"))) {
	if ((pid = fork()) == 0) {
	    fclose(fp);
	    if (setsid() >= 0)
		status = callback(user_data);
	    else {
		log_syserr("unable to set session id");
		status = 7;
	    }
	} else {
	    if (pid == -1) {
		log_syserr("unable to fork");
		status = 6;
	    } else
		fprintf(fp, "%d\n", pid);
	    fclose(fp);
	}
    } else {
	log_syserr("unable to open pid file '%s' for writing");
	status = 5;
    }
    return status;
}    

int cc_daemon_main(const char *dir, const char *log_file,
		   daemon_cb callback, void *user_data)
{
    int status = 0;
    int fd;
 
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
		status = callback(user_data);
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
