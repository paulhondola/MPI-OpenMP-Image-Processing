#ifndef __CONVOLUTION_H__
#define __CONVOLUTION_H__

#include "../bmp/bmp_io.h"
#include "../config/kernel.h"
#include "../errors/errors.h"

/**
 * Applies a convolution kernel to an image.
 * @param img Pointer to the Image structure to modify
 * @param kernel The convolution kernel to apply
 * @return app_error code:
 *         - SUCCESS: Convolution completed successfully
 *         - ERR_MEM_ALLOC: Memory allocation failed for output image
 */
app_error convolve_serial(Image *img, Kernel kernel, double *elapsed_time);
app_error convolve_parallel_multithreaded(Image *img, Kernel kernel,
                                          double *elapsed_time);
app_error convolve_parallel_distributed_filesystem(Image *img, Kernel kernel,
                                                   double *elapsed_time);
app_error convolve_parallel_shared_filesystem(Image *img, Kernel kernel,
                                              double *elapsed_time);

app_error check_images_match(Image *img1, Image *img2);

#endif
