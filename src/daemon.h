#ifndef DAEMON_INC
#define DAEMON_INC

typedef struct _cc_ctx cc_ctx_t;

typedef int (*daemon_cb)(cc_ctx_t * ctx);

extern int cc_daemon(const char *dir, const char *log_file, const char *pid_file, daemon_cb callback, cc_ctx_t *ctx);

#endif
