#ifndef MAPFILE_INC
#define MAPFILE_INC

#include <stddef.h>

typedef enum {
    MF_SUCCESS,
    MF_IGNORE,
    MF_STOP,
    MF_FAIL
} mf_status;

typedef mf_status(*mf_callback) (void *user_data, const void *file_data, size_t file_size);

extern mf_status mapfile(const char *filename, void *user_data, mf_callback callback);

#endif
