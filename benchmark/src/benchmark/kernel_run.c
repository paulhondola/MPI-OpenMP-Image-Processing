#include "kernel_run.h"
#include "../config/files.h"
#include "../file_utils/file_utils.h"
#include <limits.h>
#include <mpi.h>
#include <stdio.h>

app_error create_implementation_directories(const char *kernel_dir) {

  int rank;
  app_error err = SUCCESS;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (rank != 0)
    return SUCCESS;

  char path[PATH_MAX];

  // Create kernel folder
  snprintf(path, PATH_MAX, "%s/%s", IMAGES_FOLDER, kernel_dir);
  err = create_directory(path);
  if (err)
    return err;

  for (int i = 0; i < CONVOLUTION_MODES; i++) {
    snprintf(path, PATH_MAX, "%s/%s/%s", IMAGES_FOLDER, kernel_dir,
             IMPLEMENTATION_FOLDERS[i]);
    err = create_directory(path);
    if (err)
      return err;
  }
  return SUCCESS;
}

// Level 0: Create directories
app_error create_directories(void) {
  int rank;
  app_error err = SUCCESS;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  if (rank != 0)
    return SUCCESS;

  // Create base images folder
  err = create_directory(IMAGES_FOLDER);
  if (err)
    return err;

  for (int k = 0; k < NUM_KERNELS; k++) {
    err = create_implementation_directories(CONV_KERNELS[k].name);
    if (err)
      return err;
  }
  return SUCCESS;
}

// Level 1: Run a single kernel on a single file
app_error run_single_kernel(Image *img, const char *img_name, Kernel kernel,
                            const char *benchmark_type_folder,
                            convolve_function cv_fn, double *elapsed_time) {
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  if (rank == 0)
    printf("\tApplying kernel: %s\n", kernel.name);

  app_error err = cv_fn(img, kernel, elapsed_time);
  if (err) {
    if (rank == 0)
      fprintf(stderr, "\tError executing kernel %s: %d\n", kernel.name, err);
    return err;
  }

  // Only rank 0 saves the image and logs
  if (rank == 0) {
    printf("\tTime: %.6f s\n", *elapsed_time);
    char output_path[PATH_MAX];
    snprintf(output_path, PATH_MAX, "%s/%s/%s/%s", IMAGES_FOLDER, kernel.name,
             benchmark_type_folder, img_name);

    err = save_BMP(img, output_path);
    if (err) {
      fprintf(stderr, "\t\tError: Could not save to %s: %s\n", output_path,
              get_error_string(err));
      return err;
    }

    printf("\t\tSaved to: %s\n\n", output_path);
  }
  return SUCCESS;
}

// Level 2: Run all kernels on a single file
app_error run_all_kernels(Image *base_img, const char *img_name,
                          const char *benchmark_type_folder,
                          convolve_function cv_fn,
                          double elapsed_time[KERNEL_TYPES]) {
  app_error err = SUCCESS;
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  for (int k = 0; k < KERNEL_TYPES; k++) {
    Image *working_img = NULL;

    // Rank 0 creates a copy, others just pass NULL (or whatever they possess)
    // Actually, cv_fn needs to handle NULL input for non-root ranks if the
    // logic is fully distributed. But for simplicity, we let rank 0 copy.
    if (rank == 0) {
      err = copy_image(base_img, &working_img);
      if (err) {
        fprintf(stderr, "\tError: Could not copy image: %s\n",
                get_error_string(err));
        return err;
      }
    } else {
      // Non-root ranks might need a dummy image structure or just NULL
      // depending on cv_fn.
      // But we will let cv_fn handle NULL img for non-root, OR we create a
      // dummy wrapper. Let's pass NULL and assume cv_fn handles it or logic
      // distributes from root.
      working_img = NULL;
    }

    err = run_single_kernel(working_img, img_name, CONV_KERNELS[k],
                            benchmark_type_folder, cv_fn, &elapsed_time[k]);

    // Always free the working copy
    if (rank == 0)
      free_BMP(working_img);

    if (err)
      return err;
  }
  return SUCCESS;
}

// Level 3: Run on all existing files
app_error run_all_files(const char *benchmark_type_folder,
                        convolve_function cv_fn,
                        double elapsed_time[BENCHMARK_FILES][KERNEL_TYPES]) {
  app_error err = create_directories();
  if (err)
    return err;

  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  for (int f = 0; f < BENCHMARK_FILES; f++) {
    const char *img_name = files[f];

    if (rank == 0) {
      char input_path[PATH_MAX];
      snprintf(input_path, PATH_MAX, "%s/%s/%s", IMAGES_FOLDER, BASE_FOLDER,
               img_name);
      printf("\nProcessing file: %s\n", input_path);
    }

    // Load base image once per file
    Image *base_img = NULL;

    if (rank == 0) {
      char input_path[PATH_MAX];
      snprintf(input_path, PATH_MAX, "%s/%s/%s", IMAGES_FOLDER, BASE_FOLDER,
               img_name);
      err = read_BMP(&base_img, input_path);
      if (err) {
        fprintf(stderr, "\tError: Could not read base file %s: %s\n",
                input_path, get_error_string(err));
        return err;
      }
    }

    // Run all kernels on this file
    err = run_all_kernels(base_img, img_name, benchmark_type_folder, cv_fn,
                          elapsed_time[f]);

    // Free base image
    if (rank == 0)
      free_BMP(base_img);

    if (err)
      return err;
  }
  return SUCCESS;
}
