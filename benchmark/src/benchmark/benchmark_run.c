#include "benchmark_run.h"
#include "../bmp/bmp_io.h"
#include "../config/files.h"
#include "../config/kernel.h"
#include "../convolution/convolution.h"
#include "kernel_run.h"
#include <limits.h>
#include <mpi.h>

app_error run_benchmark_serial(void) {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (rank == 0) {
    printf("\n--- Starting Serial Benchmark ---\n");
    app_error err =
        run_all_files(SERIAL_FOLDER, convolve_serial, benchmark_data[0]);
    return err;
  }
  return SUCCESS;
}

app_error run_benchmark_parallel_multithreaded(void) {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (rank == 0) {
    printf("\n--- Starting Parallel Benchmark (Multithreaded) ---\n");
    return run_all_files(MULTITHREADED_FOLDER, convolve_parallel_multithreaded,
                         benchmark_data[1]);
  }
  return SUCCESS;
}

app_error run_benchmark_parallel_distributed_fs(void) {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (rank == 0) {
    printf("\n--- Starting Parallel Benchmark (Distributed Filesystem) ---\n");
  }
  // All ranks participate in Distributed FS benchmark
  return run_all_files(DISTRIBUTED_FOLDER,
                       convolve_parallel_distributed_filesystem,
                       benchmark_data[2]);
}

app_error run_benchmark_parallel_shared_fs(void) {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (rank == 0) {
    printf("\n--- Starting Parallel Benchmark (Shared Filesystem) ---\n");
  }
  return run_all_files(SHARED_FOLDER, convolve_parallel_shared_filesystem,
                       benchmark_data[3]);
}

app_error run_benchmark_task_pool(void) {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (rank == 0) {
    printf("\n--- Starting Parallel Benchmark (Task Pool) ---\n");
  }
  return run_all_files(TASK_POOL_FOLDER, convolve_parallel_task_pool,
                       benchmark_data[4]);
}
