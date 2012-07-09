#ifndef PARSEFILE_H
#define PARSEFILE_H

#include <regex.h>

typedef enum {
    PF_SUCCESS,
    PF_STOP,
    PF_FAIL
} pf_status;

typedef enum {
    PF_FORWARD,
    PF_BACKWARD
} pf_dir;

typedef struct _pf_sample {
    char   timestamp[16];
    double temp;
    int    sensor;
    double watts;
} pf_sample;

typedef struct _pf_context pf_context;

typedef pf_status (*pf_file_cb)(pf_context *ctx, void *data, size_t size);
typedef pf_status (*pf_sample_cb)(pf_context *ctx, pf_sample *smp);

struct _pf_context {
    pf_file_cb   file_cb;
    pf_sample_cb sample_cb;
    regex_t      preg;
    void         *user_data;
};

extern pf_context *pf_new(void);
extern void pf_free(pf_context *ctx);

extern pf_status pf_parse_line(pf_context *ctx, const char *line, const char *end);
extern pf_status pf_parse_forward(pf_context *ctx, void *data, size_t size);
extern pf_status pf_parse_backward(pf_context *ctx, void *data, size_t size);
extern pf_status pf_parse_file(pf_context *ctx, const char *filename);

#endif
