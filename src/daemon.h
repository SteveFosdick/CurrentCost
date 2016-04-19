#ifndef DAEMON_INC
#define DAEMON_INC

typedef struct _cc_ctx cc_ctx_t;

typedef int (*daemon_cb) (cc_ctx_t *ctx);

extern int cc_daemon_fork(const char *pid_file, daemon_cb cb, cc_ctx_t *ctx);
extern int cc_daemon_main(const char *dir, const char *log_file,
			  daemon_cb callback, cc_ctx_t *ctx);

#endif
