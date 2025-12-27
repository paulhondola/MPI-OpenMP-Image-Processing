#include "benchmark_io.h"
#include "../bmp/bmp_io.h"
#include "../config/files.h"
#include "../config/kernel.h"
#include "../file_utils/file_utils.h"
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>

app_error init_csv_files(BenchmarkConfig config, bool run_all) {
  app_error err = SUCCESS;
  if (run_all) {
    err = init_benchmark_csv(MULTI_RUN_CSV_FILE, MULTI_RUN_CSV_HEADER);
    if (!err) {
      err = init_benchmark_csv(SPEEDUP_CSV_FILE, SPEEDUP_CSV_HEADER);
    }
  } else {
    if (config.run_serial)
      err = init_benchmark_csv(SERIAL_CSV_FILE, SINGLE_RUN_CSV_HEADER);
    if (!err && config.run_multithreaded)
      err = init_benchmark_csv(MULTITHREADED_CSV_FILE, SINGLE_RUN_CSV_HEADER);
    if (!err && config.run_distributed)
      err = init_benchmark_csv(DISTRIBUTED_CSV_FILE, SINGLE_RUN_CSV_HEADER);
    if (!err && config.run_shared)
      err = init_benchmark_csv(SHARED_CSV_FILE, SINGLE_RUN_CSV_HEADER);
    if (!err && config.run_task_pool)
      err = init_benchmark_csv(TASK_POOL_CSV_FILE, SINGLE_RUN_CSV_HEADER);
  }
  return err;
}

app_error log_kernel_results(int file_number, int kernel_number, int comm_size,
                             BenchmarkConfig config, bool run_all, int width,
                             int height) {
  app_error err = SUCCESS;
  double serial_time = -1;
  if (config.run_serial)
    serial_time = benchmark_data[0][file_number][kernel_number];

  double multithreaded_time = -1;
  if (config.run_multithreaded)
    multithreaded_time = benchmark_data[1][file_number][kernel_number];

  double distributed_time = -1;
  if (config.run_distributed)
    distributed_time = benchmark_data[2][file_number][kernel_number];

  double shared_time = -1;
  if (config.run_shared)
    shared_time = benchmark_data[3][file_number][kernel_number];

  double task_pool_time = -1;
  if (config.run_task_pool)
    task_pool_time = benchmark_data[4][file_number][kernel_number];

  if (run_all) {
    err = append_benchmark_result(
        MULTI_RUN_CSV_FILE, width * height, CONV_KERNELS[kernel_number].size,
        comm_size, config.omp_threads, serial_time, multithreaded_time,
        distributed_time, shared_time, task_pool_time);
    if (err != SUCCESS)
      return err;
  } else {
    if (config.run_serial) {
      err = append_single_benchmark_result(
          SERIAL_CSV_FILE, width * height, CONV_KERNELS[kernel_number].size,
          comm_size, config.omp_threads, serial_time);
      if (err != SUCCESS)
        return err;
    }
    if (config.run_multithreaded) {
      err = append_single_benchmark_result(
          MULTITHREADED_CSV_FILE, width * height,
          CONV_KERNELS[kernel_number].size, comm_size, config.omp_threads,
          multithreaded_time);
      if (err != SUCCESS)
        return err;
    }
    if (config.run_distributed) {
      err = append_single_benchmark_result(DISTRIBUTED_CSV_FILE, width * height,
                                           CONV_KERNELS[kernel_number].size,
                                           comm_size, config.omp_threads,
                                           distributed_time);
      if (err != SUCCESS)
        return err;
    }
    if (config.run_shared) {
      err = append_single_benchmark_result(
          SHARED_CSV_FILE, width * height, CONV_KERNELS[kernel_number].size,
          comm_size, config.omp_threads, shared_time);
      if (err != SUCCESS)
        return err;
    }
    if (config.run_task_pool) {
      err = append_single_benchmark_result(
          TASK_POOL_CSV_FILE, width * height, CONV_KERNELS[kernel_number].size,
          comm_size, config.omp_threads, task_pool_time);
      if (err != SUCCESS)
        return err;
    }
  }
  return SUCCESS;
}

app_error log_speedup_results(int f, int k, int comm_size,
                              BenchmarkConfig config, int width, int height) {
  double serial_time = benchmark_data[0][f][k];
  double multithreaded_time = benchmark_data[1][f][k];
  double distributed_time = benchmark_data[2][f][k];
  double shared_time = benchmark_data[3][f][k];
  double task_pool_time = benchmark_data[4][f][k];

  // Calculate speedups
  // Speedup = Serial Time / Parallel Time
  // serial_speedup is always 1.0 (baseline)
  double serial_speedup = 1.0;
  double multithreaded_speedup = 0.0;
  double distributed_speedup = 0.0;
  double shared_speedup = 0.0;
  double task_pool_speedup = 0.0;

  if (multithreaded_time > 0)
    multithreaded_speedup = serial_time / multithreaded_time;
  if (distributed_time > 0)
    distributed_speedup = serial_time / distributed_time;
  if (shared_time > 0)
    shared_speedup = serial_time / shared_time;
  if (task_pool_time > 0)
    task_pool_speedup = serial_time / task_pool_time;

  return append_benchmark_result(
      SPEEDUP_CSV_FILE, width * height, CONV_KERNELS[k].size, comm_size,
      config.omp_threads, serial_speedup, multithreaded_speedup,
      distributed_speedup, shared_speedup, task_pool_speedup);
}

app_error write_benchmark_results(int comm_size, BenchmarkConfig config) {
  // Determine if we are running ALL benchmarks
  bool run_all = config.run_serial && config.run_multithreaded &&
                 config.run_distributed && config.run_shared &&
                 config.run_task_pool;

  // Initialize necessary CSV files
  app_error err = init_csv_files(config, run_all);
  if (err != SUCCESS) {
    fprintf(stderr, "Failed to initialize CSV file(s): %s\n",
            get_error_string(err));
    return err;
  }

  for (int f = 0; f < BENCHMARK_FILES; f++) {
    // Read file to get dimensions
    char input_path[PATH_MAX];
    snprintf(input_path, PATH_MAX, "%s/%s/%s", IMAGES_FOLDER, BASE_FOLDER,
             files[f]);
    Image *img = NULL;
    if (read_BMP(&img, input_path) == SUCCESS) {
      int width = img->width;
      int height = img->height;

      for (int k = 0; k < KERNEL_TYPES; k++) {
        err =
            log_kernel_results(f, k, comm_size, config, run_all, width, height);

        if (err != SUCCESS) {
          fprintf(stderr,
                  "Failed to append result for file %s, kernel %d: %s\n",
                  files[f], k, get_error_string(err));
          free_BMP(img);
          return err;
        }

        if (run_all) {
          err = log_speedup_results(f, k, comm_size, config, width, height);
          if (err != SUCCESS) {
            fprintf(stderr,
                    "Failed to append speedup result for file %s, kernel %d: "
                    "%s\n",
                    files[f], k, get_error_string(err));
            free_BMP(img);
            return err;
          }
        }
      }
      free_BMP(img);
    } else {
      fprintf(stderr, "Failed to read image %s for info\n", input_path);
    }
  }

  if (run_all) {
    printf("\nBenchmark results written to %s\n", MULTI_RUN_CSV_FILE);
    printf("Speedup results written to %s\n", SPEEDUP_CSV_FILE);
  } else {
    printf("\nBenchmark results written to separate files based on executed "
           "modes.\n");
  }

  return err;
}
