#include "convolution.h"
#include <mpi.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

unsigned char cast_to_pixel_value(double val) {
  return (unsigned char)(val < 0 ? 0 : (val > 255 ? 255 : val));
}

void clamp_pixel(Pixel *p, double r, double g, double b) {
  p->r = cast_to_pixel_value(r);
  p->g = cast_to_pixel_value(g);
  p->b = cast_to_pixel_value(b);
}

void clamp_to_boundary(int *px, int *py, int width, int height) {
  if (*px < 0)
    *px = 0;
  if (*px >= width)
    *px = width - 1;
  if (*py < 0)
    *py = 0;
  if (*py >= height)
    *py = height - 1;
}

app_error convolve_serial(Image *img, Kernel kernel, double *elapsed_time) {
  double start_time = MPI_Wtime();
  int width = img->width;
  int height = img->height;
  int k_size = kernel.size;
  int half_k = k_size / 2;

  Pixel *output = alloc_pixel(width, height);
  if (!output) {
    return ERR_MEM_ALLOC;
  }

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      double r_acc = 0, g_acc = 0, b_acc = 0;

      for (int ky = 0; ky < k_size; ky++) {
        for (int kx = 0; kx < k_size; kx++) {
          int py = y + ky - half_k;
          int px = x + kx - half_k;

          clamp_to_boundary(&px, &py, width, height);

          Pixel p = img->data[py * width + px];
          double k_val = kernel.data[ky * k_size + kx];

          r_acc += p.r * k_val;
          g_acc += p.g * k_val;
          b_acc += p.b * k_val;
        }
      }

      Pixel out_p;
      clamp_pixel(&out_p, r_acc, g_acc, b_acc);
      output[y * width + x] = out_p;
    }
  }

  free(img->data);
  img->data = output;

  double end_time = MPI_Wtime();
  if (elapsed_time != NULL) {
    *elapsed_time = end_time - start_time;
  }

  return SUCCESS;
}

app_error convolve_parallel_multithreaded(Image *img, Kernel kernel,
                                          double *elapsed_time) {
  double start_time = MPI_Wtime();
  int width = img->width;
  int height = img->height;
  int k_size = kernel.size;
  int half_k = k_size / 2;

  Pixel *output = alloc_pixel(width, height);
  if (!output) {
    return ERR_MEM_ALLOC;
  }

  Pixel *restrict output_data = output;
  const Pixel *restrict input_data = img->data;
  const double *restrict kernel_data = kernel.data;

#pragma omp parallel for collapse(2) schedule(dynamic)
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      double r_acc = 0, g_acc = 0, b_acc = 0;

      for (int ky = 0; ky < k_size; ky++) {
        for (int kx = 0; kx < k_size; kx++) {
          int py = y + ky - half_k;
          int px = x + kx - half_k;

          clamp_to_boundary(&px, &py, width, height);

          Pixel p = input_data[py * width + px];
          double k_val = kernel_data[ky * k_size + kx];

          r_acc += p.r * k_val;
          g_acc += p.g * k_val;
          b_acc += p.b * k_val;
        }
      }

      Pixel out_p;
      clamp_pixel(&out_p, r_acc, g_acc, b_acc);
      output_data[y * width + x] = out_p;
    }
  }

  free(img->data);
  img->data = output;

  double end_time = MPI_Wtime();
  if (elapsed_time != NULL)
    *elapsed_time = end_time - start_time;

  return SUCCESS;
}

void get_chunk_metadata(int height, int rank, int size, int *start_y,
                        int *local_h) {
  int rows_per_proc = height / size;
  int remainder = height % size;

  if (rank < remainder) {
    *local_h = rows_per_proc + 1;
    *start_y = rank * (*local_h);
  } else {
    *local_h = rows_per_proc;
    *start_y = rank * rows_per_proc + remainder;
  }
}

void exchange_halos(Pixel *data, int width, int local_h, int halo_size,
                    int rank, int size) {
  int top_neighbor = (rank == 0) ? MPI_PROC_NULL : rank - 1;
  int bottom_neighbor = (rank == size - 1) ? MPI_PROC_NULL : rank + 1;
  MPI_Status status;

  // Send top real rows UP, Receive from bottom neighbor into bottom halo
  // We send 'halo_size' rows starting at data[halo_size * width].
  // We receive into data[(local_h + halo_size) * width].
  MPI_Sendrecv(data + halo_size * width, halo_size * width * sizeof(Pixel),
               MPI_BYTE, top_neighbor, 0, data + (local_h + halo_size) * width,
               halo_size * width * sizeof(Pixel), MPI_BYTE, bottom_neighbor, 0,
               MPI_COMM_WORLD, &status);

  // Send bottom real rows DOWN, Receive from top neighbor into top halo
  // We send 'halo_size' rows starting at data[(local_h) * width].
  // We receive into data[0].
  MPI_Sendrecv(data + local_h * width, halo_size * width * sizeof(Pixel),
               MPI_BYTE, bottom_neighbor, 1, data,
               halo_size * width * sizeof(Pixel), MPI_BYTE, top_neighbor, 1,
               MPI_COMM_WORLD, &status);
}

app_error convolve_parallel_distributed_filesystem(Image *img, Kernel kernel,
                                                   double *elapsed_time) {
  double start_time = MPI_Wtime();
  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  int width, height, k_size;

  // 1. Broadcast Dimensions
  if (rank == 0) {
    width = img->width;
    height = img->height;
    k_size = kernel.size;
  }
  MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&k_size, 1, MPI_INT, 0, MPI_COMM_WORLD);

  // Re-allocate kernel data on non-root (if kernel struct data pointer is null
  // or invalid) But Kernel struct is passed by value. Contains pointer.
  // The pointer 'kernel.data' is only valid on Rank 0.
  // We need to broadcast kernel content.
  double *local_kernel_data = NULL;
  if (rank == 0) {
    local_kernel_data = (double *)kernel.data; // Already valid
  } else {
    local_kernel_data = (double *)malloc(k_size * k_size * sizeof(double));
    if (!local_kernel_data)
      return ERR_MEM_ALLOC;
  }
  MPI_Bcast(local_kernel_data, k_size * k_size, MPI_DOUBLE, 0, MPI_COMM_WORLD);

  // 2. Calculate Chunk Splits
  int local_h, start_y;
  get_chunk_metadata(height, rank, size, &start_y, &local_h);

  // Prepare Scatterv counts
  int *sendcounts = NULL;
  int *displs = NULL;
  if (rank == 0) {
    sendcounts = malloc(size * sizeof(int));
    displs = malloc(size * sizeof(int));
    int current_disp = 0;
    for (int r = 0; r < size; r++) {
      int r_h, r_xy;
      get_chunk_metadata(height, r, size, &r_xy, &r_h);
      sendcounts[r] = r_h * width * sizeof(Pixel);
      displs[r] = current_disp;
      current_disp += sendcounts[r];
    }
  }

  // 3. Allocate Local Buffer (w/ Halo)
  int halo_size = k_size / 2;
  int local_buffer_height = local_h + 2 * halo_size;
  Pixel *local_data =
      (Pixel *)malloc(local_buffer_height * width * sizeof(Pixel));
  Pixel *local_output = (Pixel *)malloc(local_h * width * sizeof(Pixel));

  if (!local_data || !local_output) {
    if (rank != 0)
      free(local_kernel_data);
    return ERR_MEM_ALLOC;
  }

  // 4. Scatter Data (into the "middle" of local_data, skipping top halo)
  Pixel *scatter_target = local_data + halo_size * width;
  // Note: MPI_Scatterv sends bytes because we used sizeof(Pixel) in counts
  MPI_Scatterv((rank == 0) ? img->data : NULL, sendcounts, displs, MPI_BYTE,
               scatter_target, local_h * width * sizeof(Pixel), MPI_BYTE, 0,
               MPI_COMM_WORLD);

  if (rank == 0) {
    free(sendcounts);
    free(displs);
  }

  // 5. Fill Boundaries / Exchange Halos
  exchange_halos(local_data, width, local_h, halo_size, rank, size);

  // Manual clamp fill for global boundaries
  if (rank == 0) {
    // Fill top halo with first row
    for (int h = 0; h < halo_size; h++) {
      memcpy(local_data + h * width, local_data + halo_size * width,
             width * sizeof(Pixel));
    }
  }
  if (rank == size - 1) {
    // Fill bottom halo with last row
    for (int h = 0; h < halo_size; h++) {
      memcpy(local_data + (local_h + halo_size + h) * width,
             local_data + (local_h + halo_size - 1) * width,
             width * sizeof(Pixel));
    }
  }

  // 6. Compute Convolution (OpenMP)
  int half_k = halo_size;

#pragma omp parallel for collapse(2) schedule(dynamic)
  for (int y = 0; y < local_h; y++) {
    for (int x = 0; x < width; x++) {
      double r_acc = 0, g_acc = 0, b_acc = 0;

      for (int ky = 0; ky < k_size; ky++) {
        for (int kx = 0; kx < k_size; kx++) {
          // Local buffer coordinates:
          // Center row is 'y + halo_size'
          // Neighbor row is 'y + halo_size + (ky - half_k)'
          int py = y + halo_size + ky - half_k;
          int px = x + kx - half_k;

          // Clamp X coordinate
          if (px < 0)
            px = 0;
          if (px >= width)
            px = width - 1;

          // Y coordinate is already safely inside local_data thanks to halos
          // BUT: Are we sure 'py' is valid?
          // py ranges from [0, local_buffer_height-1] essentially.
          // y=0, ky=0 -> py = halo_size - half_k = 0. Valid.
          // y=local_h-1, ky=k_size-1 -> py = local_h-1 + halo_size + half_k =
          // local_h + 2*half_k - 1. Valid.

          Pixel p = local_data[py * width + px];
          double k_val = local_kernel_data[ky * k_size + kx];

          r_acc += p.r * k_val;
          g_acc += p.g * k_val;
          b_acc += p.b * k_val;
        }
      }

      Pixel out_p;
      clamp_pixel(&out_p, r_acc, g_acc, b_acc);
      local_output[y * width + x] = out_p;
    }
  }

  // 7. Gather Results
  // Re-calculate counts for Gatherv
  int *recvcounts = NULL;
  int *rdispls = NULL;
  if (rank == 0) {
    recvcounts = malloc(size * sizeof(int));
    rdispls = malloc(size * sizeof(int));
    int current_disp = 0;
    for (int r = 0; r < size; r++) {
      int r_h, r_xy;
      get_chunk_metadata(height, r, size, &r_xy, &r_h);
      recvcounts[r] = r_h * width * sizeof(Pixel);
      rdispls[r] = current_disp;
      current_disp += recvcounts[r];
    }
  }

  MPI_Gatherv(local_output, local_h * width * sizeof(Pixel), MPI_BYTE,
              (rank == 0) ? img->data : NULL, recvcounts, rdispls, MPI_BYTE, 0,
              MPI_COMM_WORLD);

  // 8. Cleanup
  free(local_data);
  free(local_output);
  if (rank != 0)
    free(local_kernel_data);
  if (rank == 0) {
    free(recvcounts);
    free(rdispls);
  }

  double end_time = MPI_Wtime();
  if (elapsed_time != NULL)
    *elapsed_time = end_time - start_time;

  return SUCCESS;
}

app_error convolve_parallel_shared_filesystem(Image *img, Kernel kernel,
                                              double *elapsed_time) {
  return convolve_parallel_multithreaded(img, kernel, elapsed_time);
}

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
