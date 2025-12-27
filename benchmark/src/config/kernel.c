#include "kernel.h"

const double RIDGE_DATA[9] = {0, -1, 0, -1, 4, -1, 0, -1, 0};
const Kernel RIDGE_KERNEL = {"ridge", 3, RIDGE_DATA};

const double EDGE_DATA[9] = {-1, -1, -1, -1, 8, -1, -1, -1, -1};
const Kernel EDGE_KERNEL = {"edge", 3, EDGE_DATA};

const double SHARPEN_DATA[9] = {0, -1, 0, -1, 5, -1, 0, -1, 0};
const Kernel SHARPEN_KERNEL = {"sharpen", 3, SHARPEN_DATA};

const double BOXBLUR_DATA[9] = {1.0 / 9, 1.0 / 9, 1.0 / 9, 1.0 / 9, 1.0 / 9,
                                1.0 / 9, 1.0 / 9, 1.0 / 9, 1.0 / 9};
const Kernel BOXBLUR_KERNEL = {"boxblur", 3, BOXBLUR_DATA};

const double GAUSSIAN3_DATA[9] = {1.0 / 16, 2.0 / 16, 1.0 / 16,
                                  2.0 / 16, 4.0 / 16, 2.0 / 16,
                                  1.0 / 16, 2.0 / 16, 1.0 / 16};
const Kernel GAUSSIAN3_KERNEL = {"gaussblur3", 3, GAUSSIAN3_DATA};

const double GAUSSIAN5_DATA[25] = {
    1.0 / 256, 4.0 / 256,  6.0 / 256,  4.0 / 256,  1.0 / 256,
    4.0 / 256, 16.0 / 256, 24.0 / 256, 16.0 / 256, 4.0 / 256,
    6.0 / 256, 24.0 / 256, 36.0 / 256, 24.0 / 256, 6.0 / 256,
    4.0 / 256, 16.0 / 256, 24.0 / 256, 16.0 / 256, 4.0 / 256,
    1.0 / 256, 4.0 / 256,  6.0 / 256,  4.0 / 256,  1.0 / 256};
const Kernel GAUSSIAN5_KERNEL = {"gaussblur5", 5, GAUSSIAN5_DATA};

const double UNSHARP5_DATA[25] = {
    -1.0 / 256, -4.0 / 256,  -6.0 / 256,  -4.0 / 256,  -1.0 / 256,
    -4.0 / 256, -16.0 / 256, -24.0 / 256, -16.0 / 256, -4.0 / 256,
    -6.0 / 256, -24.0 / 256, 476.0 / 256, -24.0 / 256, -6.0 / 256,
    -4.0 / 256, -16.0 / 256, -24.0 / 256, -16.0 / 256, -4.0 / 256,
    -1.0 / 256, -4.0 / 256,  -6.0 / 256,  -4.0 / 256,  -1.0 / 256};
const Kernel UNSHARP5_KERNEL = {"unsharp5", 5, UNSHARP5_DATA};

const Kernel CONV_KERNELS[] = {
    RIDGE_KERNEL,     EDGE_KERNEL,      SHARPEN_KERNEL, BOXBLUR_KERNEL,
    GAUSSIAN3_KERNEL, GAUSSIAN5_KERNEL, UNSHARP5_KERNEL};

const int NUM_KERNELS = sizeof(CONV_KERNELS) / sizeof(CONV_KERNELS[0]);
