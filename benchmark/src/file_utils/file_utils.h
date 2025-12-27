#ifndef __FILE_UTILS_H__
#define __FILE_UTILS_H__

#include "../errors/errors.h"

/**
 * Initializes the benchmark CSV file with headers if it doesn't exist.
 * @param filename Name of the CSV file
 * @return app_error code:
 *         - SUCCESS: File initialized successfully
 *         - ERR_FILE_OPEN: Could not open file
 *         - ERR_FILE_IO: Error seeking or telling file position
 */
app_error init_benchmark_csv(const char *filename, const char *header);

/**
 * Appends a benchmark result row to the CSV file.
 * Automatically calculates speedup and efficiency.
 * @param filename Name of the CSV file
 * @param pixel_count Number of pixels in the image
 * @param kernel_size Size of the kernel
 * @param clusters Number of clusters (or processes)
 * @param threads Number of threads
 * @param serial_time Time taken for serial execution
 * @param multithreaded_time Time taken for multithreaded execution
 * @param distributed_time Time taken for distributed execution
 * @param shared_time Time taken for shared execution
 * @return app_error code:
 *         - SUCCESS: Result appended successfully
 *         - ERR_FILE_OPEN: Could not open file for appending
 */
app_error append_benchmark_result(const char *filename, int pixel_count,
                                  int kernel_size, int clusters, int threads,
                                  double serial_time, double multithreaded_time,
                                  double distributed_time, double shared_time,
                                  double task_pool_time);

app_error append_single_benchmark_result(const char *filename, int pixel_count,
                                         int kernel_size, int clusters,
                                         int threads, double time);

/**
 * Recursively creates all directories specified in the given path.
 * Handles both absolute and relative paths.
 * Leading and trailing slashes are ignored.
 * Only supports "/" as the path separator (Unix-like systems).
 * @param path Path to the directory
 * @return app_error code:
 *         - SUCCESS: Directory created successfully or already exists
 *         - ERR_INVALID_ARGS: Path is NULL or empty
 *         - ERR_DIR_CREATE: failed to create directory (mkdir failed)
 */
app_error create_directory(const char *path);

#endif
