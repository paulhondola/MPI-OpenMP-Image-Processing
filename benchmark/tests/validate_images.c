#include "benchmark/benchmark_run.h"
#include "bmp/bmp_io.h"
#include "config/files.h"
#include "config/kernel.h"
#include "convolution/convolution.h"
#include "errors/errors.h"
#include "file_utils/file_utils.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define RESET "\033[0m"

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

app_error verify_implmentations(const char *kernel_dir, const char *img_name,
                                int *mismatches) {
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

  verify_implementation(kernel_dir, MULTITHREADED_FOLDER, img_name, img_serial,
                        mismatches);
  verify_implementation(kernel_dir, DISTRIBUTED_FOLDER, img_name, img_serial,
                        mismatches);
  verify_implementation(kernel_dir, SHARED_FOLDER, img_name, img_serial,
                        mismatches);
  // verify_implementation(kernel_dir, TASK_POOL_FOLDER, img_name, img_serial,
  //                       mismatches);

  free_BMP(img_serial);
  return err;
}

app_error run_custom_verification() {
  printf("\n--- Starting Verification ---\n");

  int mismatches = 0;
  app_error err = SUCCESS;
  for (int f = 0; f < BENCHMARK_FILES; f++) {
    const char *img_name = files[f];
    printf("\nVerifying file: %s\n", img_name);

    for (int k = 0; k < KERNEL_TYPES; k++) {
      err = verify_implmentations(CONV_KERNELS[k].name, img_name, &mismatches);
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

#include <mpi.h>

int main(int argc, char **argv) {
  MPI_Init(&argc, &argv);
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  app_error err = SUCCESS;
  if (rank == 0) {
    err = run_custom_verification();
  }

  MPI_Finalize();
  return (err == SUCCESS) ? 0 : 1;
}
