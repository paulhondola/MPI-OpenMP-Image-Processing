#ifndef __BENCHMARK_IO_H__
#define __BENCHMARK_IO_H__

#include "../errors/errors.h"
#include "benchmark_run.h"

/**
 * @brief Writes the benchmark results to CSV file(s).
 *
 * @param comm_size The size of the MPI communicator.
 * @param config The benchmark configuration.
 * @return app_error code
 */
app_error write_benchmark_results(int comm_size, BenchmarkConfig config);

#endif
