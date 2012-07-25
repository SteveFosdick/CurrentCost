#ifndef DAEMON_INC
#define DAEMON_INC

typedef int (*daemon_cb)(void *user_data);

extern int cc_daemon(const char *dir, const char *log_file,
		     daemon_cb callback, void *user_data);

#endif
