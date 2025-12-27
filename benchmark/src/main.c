
#include "benchmark/benchmark_io.h"
#include "benchmark/benchmark_run.h"
#include <limits.h>
#include <mpi.h>
#include <omp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_THREAD_COUNT 1
#define DEFAULT_THREAD_COUNT 1

void print_usage(const char *prog_name) {
  printf("Usage: %s [options]\n", prog_name);
  printf("Options:\n");
  printf("  -t <n>  Set number of OpenMP threads (default: %d)\n",
         DEFAULT_THREAD_COUNT);
  printf("  -s      Run Serial benchmark\n");
  printf("  -m      Run Parallel Multithreaded benchmark\n");
  printf("  -d      Run Parallel Distributed Filesystem benchmark\n");
  printf("  -h      Run Parallel Shared Filesystem benchmark\n");
  printf("  -p      Run Parallel Task Pool benchmark\n");
  printf("  -a      Run All benchmarks\n");
  printf(
      "  -verify Verify the output images against the serial implementation\n");
  printf("  --help  Show this help message\n");
  printf("\nIf no mode flags are provided, Distributed mode (-d) is run by "
         "default.\n");
}

void parse_args(int argc, char **argv, BenchmarkConfig *config) {
  config->omp_threads = DEFAULT_THREAD_COUNT;
  config->run_serial = 0;
  config->run_multithreaded = 0;
  config->run_distributed = 0;
  config->run_shared = 0;
  config->run_task_pool = 0;
  config->verify = 0;

  bool flags_set = false;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--help") == 0) {
      print_usage(argv[0]);
      exit(0);
    } else if (strcmp(argv[i], "-threads") == 0 && i + 1 < argc) {
      config->omp_threads = atoi(argv[++i]);
    } else if (strcmp(argv[i], "-serial") == 0) {
      config->run_serial = 1;
      flags_set = true;
    } else if (strcmp(argv[i], "-multithreaded") == 0) {
      config->run_multithreaded = 1;
      flags_set = true;
    } else if (strcmp(argv[i], "-distributed") == 0) {
      config->run_distributed = 1;
      flags_set = true;
    } else if (strcmp(argv[i], "-shared") == 0) {
      config->run_shared = 1;
      flags_set = true;
    } else if (strcmp(argv[i], "-task_pool") == 0) {
      config->run_task_pool = 1;
      flags_set = true;
    } else if (strcmp(argv[i], "-all") == 0) {
      config->run_serial = 1;
      config->run_multithreaded = 1;
      config->run_distributed = 1;
      config->run_shared = 1;
      config->run_task_pool = 1;
      flags_set = true;
    } else if (strcmp(argv[i], "-verify") == 0) {
      config->verify = 1;
    } else {
      fprintf(stderr, "Unknown argument: %s\n", argv[i]);
      print_usage(argv[0]);
      exit(1);
    }
  }

  // exit if no flags set
  if (!flags_set) {
    print_usage(argv[0]);
    exit(1);
  }
}

void init_mpi(int argc, char **argv, int *comm_rank, int *comm_size,
              BenchmarkConfig *config) {
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, comm_rank);
  MPI_Comm_size(MPI_COMM_WORLD, comm_size);

  parse_args(argc, argv, config);

  omp_set_num_threads(config->omp_threads);
}

app_error run_benchmarks(BenchmarkConfig config) {
  int comm_rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);
  app_error err = SUCCESS;

  if (config.run_serial) {
    err = run_benchmark_serial();
    if (err != SUCCESS) {
      if (comm_rank == 0)
        fprintf(stderr, "Serial benchmark failed with error: %s\n",
                get_error_string(err));
      return err;
    }
  }

  if (config.run_multithreaded) {
    err = run_benchmark_parallel_multithreaded();
    if (err != SUCCESS) {
      if (comm_rank == 0)
        fprintf(stderr,
                "Parallel benchmark (Multithreaded) failed with error: %s\n",
                get_error_string(err));
      return err;
    }
  }

  if (config.run_distributed) {
    err = run_benchmark_parallel_distributed_fs();
    if (err != SUCCESS) {
      if (comm_rank == 0)
        fprintf(stderr,
                "Parallel benchmark (Distributed Filesystem) failed with "
                "error: %s\n",
                get_error_string(err));
      return err;
    }
  }

  /*
  if (config.run_shared) {
    err = run_benchmark_parallel_shared_fs();
    if (err != SUCCESS) {
      if (comm_rank == 0)
        fprintf(
            stderr,
            "Parallel benchmark (Shared Filesystem) failed with error: %s\n",
            get_error_string(err));
      return err;
    }
  }

  if (config.run_task_pool) {
    err = run_benchmark_task_pool();
    if (err != SUCCESS) {
      if (comm_rank == 0)
        fprintf(stderr,
                "Parallel benchmark (Task Pool) failed with error: %s\n",
                get_error_string(err));
      return err;
    }
  }
  */

  return err;
}

void print_mode(BenchmarkConfig config, int comm_size) {
  if (comm_size == 0) {
    printf("Running with %d MPI processes and %d OpenMP threads per process\n",
           comm_size, config.omp_threads);
    if (config.run_serial)
      printf("Mode: Serial\n");
    if (config.run_multithreaded)
      printf("Mode: Parallel Multithreaded\n");
    if (config.run_distributed)
      printf("Mode: Parallel Distributed Filesystem\n");
    if (config.run_shared)
      printf("Mode: Parallel Shared Filesystem\n");
    if (config.run_task_pool)
      printf("Mode: Parallel Task Pool\n");
  }
}

int main(int argc, char **argv) {
  int comm_rank, comm_size;
  BenchmarkConfig config;
  init_mpi(argc, argv, &comm_rank, &comm_size, &config);

  print_mode(config, comm_size);

  // Run all benchmarks
  app_error err = run_benchmarks(config);

  if (err != SUCCESS) {
    MPI_Finalize();
    return err;
  }

  if (comm_rank == 0) {
    if (config.verify) {
      // Run verification
      err = run_verification(config);

      if (err != SUCCESS) {
        fprintf(stderr, "Verification failed with error: %s\n",
                get_error_string(err));
        MPI_Finalize();
        return err;
      }
    }

    // Write results to CSV
    err = write_benchmark_results(comm_size, config);

    if (err != SUCCESS) {
      fprintf(stderr, "Failed to write benchmark results: %s\n",
              get_error_string(err));
      MPI_Finalize();
      return err;
    }
  }

  MPI_Finalize();
  return 0;
}
