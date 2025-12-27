#ifndef __KERNEL_RUN_H__
#define __KERNEL_RUN_H__

#include "../bmp/bmp_io.h"
#include "../config/files.h"
#include "../config/kernel.h"
#include "../errors/errors.h"

// Callback type for convolution functions
typedef app_error (*convolve_function)(Image *img, Kernel ker,
                                       double *elapsed_time);

/**
 * @brief Runs a single kernel on an image and saves the output.
 *
 * @param img The image to process.
 * @param img_name The name of the original image file.
 * @param kernel The kernel to apply.
 * @param benchmark_type_folder The subfolder name (Serial or Parallel) to save
 * validation output.
 * @param cv_fn The convolution function to use.
 * @return app_error
 */
app_error run_single_kernel(Image *img, const char *img_name, Kernel kernel,
                            const char *benchmark_type_folder,
                            convolve_function cv_fn, double *elapsed_time);

/**
 * @brief Runs all defined kernels on an image.
 *
 * @param base_img The source image.
 * @param img_name The name of the image file.
 * @param benchmark_type_folder The subfolder name to save validation output.
 * @param cv_fn The convolution function to use.
 * @return app_error
 */
app_error run_all_kernels(Image *img, const char *img_name,
                          const char *benchmark_type_folder,
                          convolve_function cv_fn,
                          double elapsed_time[KERNEL_TYPES]);

/**
 * @brief Runs all defined kernels on all images in the base folder.
 *
 * @param benchmark_type_folder The subfolder name to save validation output.
 * @param cv_fn The convolution function to use.
 * @return app_error
 */
app_error run_all_files(const char *benchmark_type_folder,
                        convolve_function cv_fn,
                        double elapsed_time[BENCHMARK_FILES][KERNEL_TYPES]);

#endif
