#include "benchmark_run.h"
#include "../bmp/bmp_io.h"
#include "../config/files.h"
#include "../config/kernel.h"
#include "../convolution/convolution.h"
#include "kernel_run.h"
#include <limits.h>
#include <mpi.h>

#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define RESET "\033[0m"

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
    return run_all_files(SHARED_FOLDER, convolve_parallel_shared_filesystem,
                         benchmark_data[3]);
  }
  return SUCCESS;
}

app_error run_benchmark_task_pool(void) {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (rank == 0) {
    printf("\n--- Starting Parallel Benchmark (Task Pool) ---\n");
    // Placeholder: using multithreaded implementation
    return run_all_files(TASK_POOL_FOLDER, convolve_parallel_multithreaded,
                         benchmark_data[4]);
  }
  return SUCCESS;
}

app_error verify_implementation(const char *kernel_dir, const char *impl_folder,
                                const char *img_name, Image *img_serial,
                                int *mismatches) {
  char path[PATH_MAX];
  snprintf(path, PATH_MAX, "%s/%s/%s/%s", IMAGES_FOLDER, kernel_dir,
           impl_folder, img_name);

  Image *img_parallel = NULL;

  app_error err = read_BMP(&img_parallel, path);
  if (err) {
    fprintf(stderr, "\tError reading %s output: %s\n", impl_folder, path);
    (*mismatches)++;
    return err;
  }

  err = check_images_match(img_serial, img_parallel);

  if (err) {
    fprintf(stderr, RED "\tMismatch found in kernel %s (%s)\n" RESET,
            kernel_dir, impl_folder);
    (*mismatches)++;
  } else {
    printf(GREEN "\t%s (%s): Match\n" RESET, kernel_dir, impl_folder);
  }

  free_BMP(img_parallel);
  return err;
}

app_error verify_implmentations(BenchmarkConfig config, const char *kernel_dir,
                                const char *img_name, int *mismatches) {
  char serial_path[PATH_MAX];

  snprintf(serial_path, PATH_MAX, "%s/%s/%s/%s", IMAGES_FOLDER, kernel_dir,
           SERIAL_FOLDER, img_name);

  Image *img_serial = NULL;

  app_error err = read_BMP(&img_serial, serial_path);

  if (err) {
    fprintf(stderr, "\tError reading serial output: %s | ERROR CODE: %s\n",
            serial_path, get_error_string(err));
    // If we can't read serial output, we can't verify anything
    return err;
  }

  if (config.run_multithreaded)
    (void)verify_implementation(kernel_dir, MULTITHREADED_FOLDER, img_name,
                                img_serial, mismatches);

  if (config.run_distributed)
    (void)verify_implementation(kernel_dir, DISTRIBUTED_FOLDER, img_name,
                                img_serial, mismatches);

  if (config.run_shared)
    (void)verify_implementation(kernel_dir, SHARED_FOLDER, img_name, img_serial,
                                mismatches);

  if (config.run_task_pool)
    (void)verify_implementation(kernel_dir, TASK_POOL_FOLDER, img_name,
                                img_serial, mismatches);

  free_BMP(img_serial);
  return err;
}

app_error run_verification(BenchmarkConfig config) {
  printf("\n--- Starting Verification ---\n");

  int mismatches = 0;
  app_error err = SUCCESS;
  for (int f = 0; f < BENCHMARK_FILES; f++) {
    const char *img_name = files[f];
    printf("\nVerifying file: %s\n", img_name);

    for (int k = 0; k < KERNEL_TYPES; k++) {
      err = verify_implmentations(config, CONV_KERNELS[k].name, img_name,
                                  &mismatches);
      if (err) {
        return err;
      }
    }
  }

  if (mismatches > 0) {
    fprintf(stderr, "\nVerification completed with %d mismatches\n",
            mismatches);
    return ERR_IMAGE_DIFFERENCE;
  }

  printf("\nVerification completed successfully. All images match.\n");
  return SUCCESS;
}
