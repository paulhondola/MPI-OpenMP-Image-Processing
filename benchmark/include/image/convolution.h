#ifndef __CONVOLUTION_H__
#define __CONVOLUTION_H__

#include "../config/errors.h"
#include "../config/kernel.h"
#include "bmp_io.h"

/**
 * Applies a convolution kernel to an image.
 * @param img Pointer to the Image structure to modify
 * @param kernel The convolution kernel to apply
 * @return app_error code:
 *         - SUCCESS: Convolution completed successfully
 *         - ERR_MEM_ALLOC: Memory allocation failed for output image
 */
app_error convolve_serial(Image *img, const char *img_name,
                          const char *benchmark_type, Kernel kernel,
                          double *elapsed_time);
app_error convolve_parallel_multithreaded(Image *img, const char *img_name,
                                          const char *benchmark_type,
                                          Kernel kernel, double *elapsed_time);
app_error convolve_parallel_distributed_filesystem(Image *img,
                                                   const char *img_name,
                                                   const char *benchmark_type,
                                                   Kernel kernel,
                                                   double *elapsed_time);
app_error convolve_parallel_shared_filesystem(Image *img, const char *img_name,
                                              const char *benchmark_type,
                                              Kernel kernel,
                                              double *elapsed_time);

app_error convolve_parallel_task_pool(Image *img, const char *img_name,
                                      const char *benchmark_type, Kernel kernel,
                                      double *elapsed_time);

app_error check_images_match(Image *img1, Image *img2);

#endif
