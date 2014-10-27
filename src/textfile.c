#include "textfile.h"

typedef struct {
    mf_callback line_cb;
    void *user_data;
} textfile_t;

mf_status tf_parse_cb_forward(void *user_data,
                              const void *file_data, size_t file_size)
{

    textfile_t *tf = (textfile_t *) user_data;
    mf_status status = MF_SUCCESS;
    const char *ptr = file_data;
    const char *end = ptr + file_size;
    const char *line = ptr;

    do {
        while (ptr < end && *ptr++ != '\n');
        status = tf->line_cb(tf->user_data, line, ptr - line - 1);
        line = ptr;
    } while (status == MF_SUCCESS && ptr < end);

    return status;
}

mf_status tf_parse_cb_backward(void *user_data,
                               const void *file_data, size_t file_size)
{

    textfile_t *tf = (textfile_t *) user_data;
    mf_status status = MF_SUCCESS;
    const char *start = file_data;
    const char *ptr = start + file_size;
    const char *line = ptr;

    do {
        while (line > start && *--line != '\n');
        status = tf->line_cb(tf->user_data, line + 1, ptr - line - 1);
        ptr = line;
    } while (status == MF_SUCCESS && ptr > start);

    return status;
}

mf_status tf_parse_file(const char *filename, void *user_data,
                        mf_callback file_cb, mf_callback line_cb)
{

    textfile_t tf;

    tf.line_cb = line_cb;
    tf.user_data = user_data;
    return mapfile(filename, &tf, file_cb);
}
