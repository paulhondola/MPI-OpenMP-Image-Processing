#include "../../include/benchmark/benchmark_run.h"
#include "../../include/config/errors.h"
#include "../../include/config/files.h"
#include "../../include/config/kernel.h"
#include "../../include/image/bmp_io.h"
#include <limits.h>

#define GREEN "\033[0;32m"
#define RED "\033[0;31m"
#define RESET "\033[0m"

app_error check_images_match(Image *img1, Image *img2) {
  if (img1->width != img2->width || img1->height != img2->height) {
    return ERR_IMAGE_DIFFERENCE;
  }

  int match = 1;

#pragma omp parallel for shared(match)
  for (int i = 0; i < img1->width * img1->height; i++) {
    if (!match)
      continue; // Early exit

    if (img1->data[i].r != img2->data[i].r ||
        img1->data[i].g != img2->data[i].g ||
        img1->data[i].b != img2->data[i].b) {
#pragma omp atomic write
      match = 0;
    }
  }

  return match ? SUCCESS : ERR_IMAGE_DIFFERENCE;
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
