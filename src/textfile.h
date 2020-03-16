#ifndef TEXTFILE_INC
#define TEXTFILE_INC

#include "mapfile.h"

extern mf_status tf_parse_cb_forward(void *user_data, const void *file_data, size_t file_size);

extern mf_status tf_parse_cb_backward(void *user_data, const void *file_data, size_t file_size);

extern mf_status tf_parse_file(const char *filename, void *user_data, mf_callback file_cb, mf_callback line_cb);

#define mf_parse_file_forward(filename, user_data, line_callback)	\
    tf_parse_file(filename, user_data, tf_parse_cb_forward, line_callback)

#define mf_parse_file_backward(filename, user_data, line_callback)	\
    tf_parse_file(filename, user_data, tf_parse_cb_backward, line_callback)

#endif
