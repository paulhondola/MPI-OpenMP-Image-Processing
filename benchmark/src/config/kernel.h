#ifndef __KERNEL_H__
#define __KERNEL_H__

/**
 * Represents a square convolution kernel.
 * Contains the name, directory, size (dimension), and the flattened data array.
 */
typedef struct {
  const char *name;
  int size;
  const double *data;
} Kernel;

extern const Kernel RIDGE_KERNEL;
extern const Kernel EDGE_KERNEL;
extern const Kernel SHARPEN_KERNEL;
extern const Kernel BOXBLUR_KERNEL;
extern const Kernel GAUSSIAN3_KERNEL;
extern const Kernel GAUSSIAN5_KERNEL;
extern const Kernel UNSHARP5_KERNEL;

extern const Kernel CONV_KERNELS[];
extern const int NUM_KERNELS;

#endif
