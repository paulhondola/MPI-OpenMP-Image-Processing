#ifndef __ERRORS_H__
#define __ERRORS_H__

typedef enum {
  SUCCESS = 0,
  ERR_FILE_IO = 1,
  ERR_FILE_OPEN,
  ERR_FILE_READ,
  ERR_FILE_WRITE,
  ERR_MEM_ALLOC,
  ERR_PATH_TOO_LONG,
  ERR_DIR_CREATE,
  ERR_NOT_A_DIR,
  ERR_INVALID_ARGS,
  ERR_BMP_HEADER,
  ERR_BMP_DATA,
  ERR_CONVOLUTION,
  ERR_MPI,
  ERR_OPENMP,
  ERR_IMAGE_DIFFERENCE,
  ERR_UNKNOWN
} app_error;

/**
 * Returns a string description of the given error code.
 * @param err The error code
 * @return String description
 */
const char *get_error_string(app_error err);

#endif
