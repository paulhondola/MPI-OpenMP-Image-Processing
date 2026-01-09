#ifndef __VALIDATE_IMAGES_H__
#define __VALIDATE_IMAGES_H__

#include "benchmark/benchmark_run.h"
#include "config/errors.h"
#include "image/bmp_io.h"
#include <limits.h>

app_error check_images_match(Image *img1, Image *img2);

app_error verify_implementation(const char *kernel_dir, const char *impl_folder,
                                const char *img_name, Image *img_serial,
                                int *mismatches);

app_error verify_implmentations(BenchmarkConfig config, const char *kernel_dir,
                                const char *img_name, int *mismatches);

app_error run_verification(BenchmarkConfig config);

#endif
