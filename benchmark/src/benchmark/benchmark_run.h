#ifndef __BENCHMARK_RUN_H__
#define __BENCHMARK_RUN_H__

#include "../errors/errors.h"

typedef struct {
  int omp_threads;
  unsigned int run_serial : 1;
  unsigned int run_multithreaded : 1;
  unsigned int run_distributed : 1;
  unsigned int run_shared : 1;
  unsigned int run_task_pool : 1;
  unsigned int verify : 1;
} BenchmarkConfig;

/**
 * @brief Runs the serial version of the image processing benchmark.
 * Executes convolution with various kernels on a test image.
 * @return app_error code:
 *         - SUCCESS: Benchmark completed successfully
 *         - ERR_DIR_CREATE: Failed to create output directories
 *         - ERR_FILE_OPEN: Failed to open input file
 *         - ERR_BMP_HEADER: Invalid BMP header in input
 *         - ERR_MEM_ALLOC: Memory allocation failed
 *         - ERR_CONVOLUTION: Convolution operation failed
 *         - ERR_FILE_WRITE: Failed to write output file
 */
app_error run_benchmark_serial(void);

/**
 * @brief Runs the parallel version of the image processing benchmark.
 * Uses OpenMP for shared memory parallelism.
 * @return app_error code
 */
app_error run_benchmark_parallel_multithreaded(void);

/**
 * @brief Runs the parallel version of the image processing benchmark.
 * Uses MPI and OpenMP for distributed memory parallelism.
 * @return app_error code
 */
app_error run_benchmark_parallel_distributed_fs(void);

/**
 * @brief Runs the parallel benchmark optimized for Shared Filesystem.
 * Each MPI rank reads its own chunk + halos directly from the file.
 * Avoids distributing chunks via messages.
 * @return app_error code
 */
app_error run_benchmark_parallel_shared_fs(void);

/**
 * @brief Runs the parallel benchmark using a Task Pool approach.
 * Currently uses multithreaded implementation as a placeholder.
 * @return app_error code
 */
app_error run_benchmark_task_pool(void);

/**
 * @brief Verifies that the serial and parallel outputs match.
 * @param config Benchmark configuration to determine which verifications to run
 * @return app_error code
 */
app_error run_verification(BenchmarkConfig config);

#endif
