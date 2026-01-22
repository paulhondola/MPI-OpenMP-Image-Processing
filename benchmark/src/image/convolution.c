#include "../../include/image/convolution.h"
#include "../../include/config/files.h"
#include "../../include/image/mpi_bmp_io.h"
#include <limits.h>
#include <mpi.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

inline unsigned char cast_to_pixel_value(double val) {
  return (unsigned char)(val < 0 ? 0 : (val > 255 ? 255 : val));
}

inline void clamp_pixel(Pixel *p, double r, double g, double b) {
  p->r = cast_to_pixel_value(r);
  p->g = cast_to_pixel_value(g);
  p->b = cast_to_pixel_value(b);
}

inline void clamp_to_boundary(int *px, int *py, int width, int height) {
  if (*px < 0)
    *px = 0;
  if (*px >= width)
    *px = width - 1;
  if (*py < 0)
    *py = 0;
  if (*py >= height)
    *py = height - 1;
}

app_error convolve_serial(Image *img, const char *img_name,
                          const char *benchmark_type, Kernel kernel,
                          double *elapsed_time) {
  (void)img_name;
  (void)benchmark_type;
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

app_error convolve_parallel_multithreaded(Image *img, const char *img_name,
                                          const char *benchmark_type,
                                          Kernel kernel, double *elapsed_time) {
  (void)img_name;
  (void)benchmark_type;
  double start_time = MPI_Wtime();
  int width = img->width;
  int height = img->height;
  int k_size = kernel.size;
  int half_k = k_size / 2;

  Pixel *restrict output = alloc_pixel(width, height);
  if (!output) {
    return ERR_MEM_ALLOC;
  }

  const Pixel *restrict input_data = img->data;
  const double *restrict kernel_data = kernel.data;

#pragma omp parallel for collapse(2) schedule(static)
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
      output[y * width + x] = out_p;
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

app_error convolve_parallel_distributed_filesystem(Image *img,
                                                   const char *img_name,
                                                   const char *benchmark_type,
                                                   Kernel kernel,
                                                   double *elapsed_time) {
  (void)img_name;
  (void)benchmark_type;
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
  int chunk_height, chunk_start_y;
  get_chunk_metadata(height, rank, size, &chunk_start_y, &chunk_height);

  // Prepare Scatterv counts
  int *scatter_counts_bytes = NULL;
  int *scatter_displs_bytes = NULL;
  if (rank == 0) {
    scatter_counts_bytes = malloc(size * sizeof(int));
    scatter_displs_bytes = malloc(size * sizeof(int));
    int current_disp = 0;
    for (int r = 0; r < size; r++) {
      int rank_chunk_h, rank_start_y;
      get_chunk_metadata(height, r, size, &rank_start_y, &rank_chunk_h);
      scatter_counts_bytes[r] = rank_chunk_h * width * sizeof(Pixel);
      scatter_displs_bytes[r] = current_disp;
      current_disp += scatter_counts_bytes[r];
    }
  }

  // 3. Allocate Local Buffer (w/ Halo)
  int halo_size = k_size / 2;
  int padded_buffer_height = chunk_height + 2 * halo_size;
  Pixel *padded_input =
      (Pixel *)malloc(padded_buffer_height * width * sizeof(Pixel));
  Pixel *output_chunk = (Pixel *)malloc(chunk_height * width * sizeof(Pixel));

  if (!padded_input || !output_chunk) {
    if (rank != 0)
      free(local_kernel_data);
    return ERR_MEM_ALLOC;
  }

  // 4. Scatter Data (into the "middle" of padded_input, skipping top halo)
  Pixel *scatter_target = padded_input + halo_size * width;
  // Note: MPI_Scatterv sends bytes because we used sizeof(Pixel) in counts
  MPI_Scatterv((rank == 0) ? img->data : NULL, scatter_counts_bytes,
               scatter_displs_bytes, MPI_BYTE, scatter_target,
               chunk_height * width * sizeof(Pixel), MPI_BYTE, 0,
               MPI_COMM_WORLD);

  if (rank == 0) {
    free(scatter_counts_bytes);
    free(scatter_displs_bytes);
  }

  // 5. Fill Boundaries / Exchange Halos
  exchange_halos(padded_input, width, chunk_height, halo_size, rank, size);

  // Manual clamp fill for global boundaries
  if (rank == 0) {
    // Fill top halo with first row
    for (int h = 0; h < halo_size; h++) {
      memcpy(padded_input + h * width, padded_input + halo_size * width,
             width * sizeof(Pixel));
    }
  }
  if (rank == size - 1) {
    // Fill bottom halo with last row
    for (int h = 0; h < halo_size; h++) {
      memcpy(padded_input + (chunk_height + halo_size + h) * width,
             padded_input + (chunk_height + halo_size - 1) * width,
             width * sizeof(Pixel));
    }
  }

  // 6. Compute Convolution (OpenMP)
  int kernel_radius = halo_size;

#pragma omp parallel for collapse(2) schedule(static)
  for (int y = 0; y < chunk_height; y++) {
    for (int x = 0; x < width; x++) {
      double sum_r = 0, sum_g = 0, sum_b = 0;

      for (int ky = 0; ky < k_size; ky++) {
        for (int kx = 0; kx < k_size; kx++) {
          // Local buffer coordinates:
          // Center row is 'y + halo_size'
          // Neighbor row is 'y + halo_size + (ky - kernel_radius)'
          int py = y + halo_size + ky - kernel_radius;
          int px = x + kx - kernel_radius;

          // Clamp X coordinate
          if (px < 0)
            px = 0;
          if (px >= width)
            px = width - 1;

          Pixel p = padded_input[py * width + px];
          double k_val = local_kernel_data[ky * k_size + kx];

          sum_r += p.r * k_val;
          sum_g += p.g * k_val;
          sum_b += p.b * k_val;
        }
      }

      Pixel out_p;
      clamp_pixel(&out_p, sum_r, sum_g, sum_b);
      output_chunk[y * width + x] = out_p;
    }
  }

  // 7. Gather Results
  // Re-calculate counts for Gatherv
  int *gather_counts_bytes = NULL;
  int *gather_displs_bytes = NULL;
  if (rank == 0) {
    gather_counts_bytes = malloc(size * sizeof(int));
    gather_displs_bytes = malloc(size * sizeof(int));
    int current_disp = 0;
    for (int r = 0; r < size; r++) {
      int rank_chunk_h, rank_start_y;
      get_chunk_metadata(height, r, size, &rank_start_y, &rank_chunk_h);
      gather_counts_bytes[r] = rank_chunk_h * width * sizeof(Pixel);
      gather_displs_bytes[r] = current_disp;
      current_disp += gather_counts_bytes[r];
    }
  }

  MPI_Gatherv(output_chunk, chunk_height * width * sizeof(Pixel), MPI_BYTE,
              (rank == 0) ? img->data : NULL, gather_counts_bytes,
              gather_displs_bytes, MPI_BYTE, 0, MPI_COMM_WORLD);

  // 8. Cleanup
  free(padded_input);
  free(output_chunk);
  if (rank != 0)
    free(local_kernel_data);
  if (rank == 0) {
    free(gather_counts_bytes);
    free(gather_displs_bytes);
  }

  double end_time = MPI_Wtime();
  if (elapsed_time != NULL)
    *elapsed_time = end_time - start_time;

  return SUCCESS;
}

app_error convolve_parallel_shared_filesystem(Image *img, const char *img_name,
                                              const char *benchmark_type,
                                              Kernel kernel,
                                              double *elapsed_time) {
  double start_time = MPI_Wtime();
  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  int width, height, k_size;
  char input_path[PATH_MAX];

  // 1. Root broadcasts dimensions and kernel
  if (rank == 0) {
    width = img->width;
    height = img->height;
    k_size = kernel.size;
    snprintf(input_path, PATH_MAX, "%s/%s/%s", IMAGES_FOLDER, BASE_FOLDER,
             img_name);
  }
  MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(&k_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
  MPI_Bcast(input_path, PATH_MAX, MPI_CHAR, 0, MPI_COMM_WORLD);

  // Broadcast Kernel Data
  double *local_kernel_data = NULL;
  if (rank == 0) {
    local_kernel_data = (double *)kernel.data;
  } else {
    local_kernel_data = (double *)malloc(k_size * k_size * sizeof(double));
    if (!local_kernel_data)
      return ERR_MEM_ALLOC;
  }
  MPI_Bcast(local_kernel_data, k_size * k_size, MPI_DOUBLE, 0, MPI_COMM_WORLD);

  // 2. Calculate Chunk Splits
  int local_h, start_y;
  get_chunk_metadata(height, rank, size, &start_y, &local_h);

  // 3. Parallel Read (MPI IO)
  // Each rank reads its own chunk of rows.
  // mpi_read_BMP_chunk allocates the image structure for us.
  Image *local_chunk_img = NULL;
  app_error err = mpi_read_BMP_chunk(&local_chunk_img, input_path, start_y,
                                     local_h, NULL, NULL);
  if (err != SUCCESS) {
    if (rank != 0)
      free(local_kernel_data);
    return err;
  }

  // 4. Prepare for Convolution (Halos)
  // We need a buffer that includes halos. local_chunk_img only has the core
  // rows.
  int halo_size = k_size / 2;
  int local_buffer_height = local_h + 2 * halo_size;
  Pixel *local_data =
      (Pixel *)malloc(local_buffer_height * width * sizeof(Pixel));
  Pixel *local_output = (Pixel *)malloc(local_h * width * sizeof(Pixel));

  if (!local_data || !local_output) {
    if (rank != 0)
      free(local_kernel_data);
    free_BMP(local_chunk_img); // This frees local_chunk_img->data too
    return ERR_MEM_ALLOC;
  }

  // Copy read data into the middle of local_data
  // chunk data is size [local_h * width]
  memcpy(local_data + halo_size * width, local_chunk_img->data,
         local_h * width * sizeof(Pixel));

  // We can free the read buffer now as we copied it
  free_BMP(local_chunk_img);

  // 5. Exchange Halos
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

#pragma omp parallel for collapse(2) schedule(static)
  for (int y = 0; y < local_h; y++) {
    for (int x = 0; x < width; x++) {
      double r_acc = 0, g_acc = 0, b_acc = 0;

      for (int ky = 0; ky < k_size; ky++) {
        for (int kx = 0; kx < k_size; kx++) {
          int py = y + halo_size + ky - half_k;
          int px = x + kx - half_k;

          if (px < 0)
            px = 0;
          if (px >= width)
            px = width - 1;

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

  // 7. Parallel Write (MPI IO)
  char output_path[PATH_MAX];
  if (rank == 0) {
    snprintf(output_path, PATH_MAX, "%s/%s/%s/%s", IMAGES_FOLDER, kernel.name,
             benchmark_type, img_name);
  }
  MPI_Bcast(output_path, PATH_MAX, MPI_CHAR, 0, MPI_COMM_WORLD);

  // Wrap local_output in an Image struct for the write function
  Image out_img_wrapper;
  out_img_wrapper.width = width;
  out_img_wrapper.height = local_h; // Height of this CHUNK
  out_img_wrapper.data = local_output;

  err = mpi_write_BMP_chunk(&out_img_wrapper, output_path, start_y, width,
                            height);
  if (err != SUCCESS) {
    // cleanup
  }

  // 8. Cleanup
  free(local_data);
  free(local_output);
  if (rank != 0)
    free(local_kernel_data);

  double end_time = MPI_Wtime();
  if (elapsed_time != NULL)
    *elapsed_time = end_time - start_time;

  return err;
}

#define TASK_CHUNK_SIZE 32
#define TAG_REQUEST 1
#define TAG_TASK 2
#define TAG_RESULT 3
#define TAG_TERMINATE 4

app_error convolve_parallel_task_pool(Image *img, const char *img_name,
                                      const char *benchmark_type, Kernel kernel,
                                      double *elapsed_time) {
  (void)img_name;
  (void)benchmark_type;
  double start_time = MPI_Wtime();
  int rank, size;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &size);

  if (size < 2) {
    if (rank == 0) {
      return convolve_serial(img, img_name, benchmark_type, kernel,
                             elapsed_time);
    }
    return SUCCESS;
  }

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

  // Broadcast Kernel Data
  double *local_kernel_data = NULL;
  if (rank == 0) {
    local_kernel_data = (double *)kernel.data;
  } else {
    local_kernel_data = (double *)malloc(k_size * k_size * sizeof(double));
    if (!local_kernel_data)
      return ERR_MEM_ALLOC;
  }
  MPI_Bcast(local_kernel_data, k_size * k_size, MPI_DOUBLE, 0, MPI_COMM_WORLD);

  int halo_size = k_size / 2;
  // printf("Rank %d: Bcast done. Width=%d, Height=%d, K_size=%d\n", rank,
  // width, height, k_size);

  // ---------------------------------------------------------
  // MASTER (PRODUCER) LOGIC
  // ---------------------------------------------------------
  if (rank == 0) {
    Pixel *output = alloc_pixel(width, height);
    if (!output)
      return ERR_MEM_ALLOC;

    int next_row = 0;
    int active_workers = size - 1;

    MPI_Status status;
    while (active_workers > 0) {
      // PROBE for any message
      // printf("Rank 0: Probing...\n");
      MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
      int source = status.MPI_SOURCE;
      int tag = status.MPI_TAG;
      // printf("Rank 0: Received tag %d from %d\n", tag, source);

      if (tag == TAG_REQUEST) {
        // Consume request msg
        int dummy;
        MPI_Recv(&dummy, 1, MPI_INT, source, TAG_REQUEST, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);

        if (next_row < height) {
          // Send Task
          int chunks_rows = TASK_CHUNK_SIZE;
          if (next_row + chunks_rows > height)
            chunks_rows = height - next_row;

          int input_start_y = next_row - halo_size;
          int input_end_y = next_row + chunks_rows - 1 + halo_size;
          int input_h = input_end_y - input_start_y + 1;

          // Prepare buffer
          Pixel *send_buf = (Pixel *)malloc(input_h * width * sizeof(Pixel));
          if (!send_buf)
            return ERR_MEM_ALLOC;

          for (int ly = 0; ly < input_h; ly++) {
            int global_y = input_start_y + ly;
            // Clamp/Pad read
            int read_y = global_y;
            if (read_y < 0)
              read_y = 0;
            if (read_y >= height)
              read_y = height - 1;

            memcpy(send_buf + ly * width, img->data + read_y * width,
                   width * sizeof(Pixel));
          }

          // Send Metadata: [start_y(global), height(output), height(input)]
          int task_meta[3] = {next_row, chunks_rows, input_h};
          MPI_Send(task_meta, 3, MPI_INT, source, TAG_TASK, MPI_COMM_WORLD);
          MPI_Send(send_buf, input_h * width * sizeof(Pixel), MPI_BYTE, source,
                   TAG_TASK, MPI_COMM_WORLD);

          free(send_buf);
          next_row += chunks_rows;
        } else {
          // Terminate
          MPI_Send(NULL, 0, MPI_INT, source, TAG_TERMINATE, MPI_COMM_WORLD);
          active_workers--;
        }
      } else if (tag == TAG_RESULT) {
        // Receive Result
        int result_meta[2];
        MPI_Recv(result_meta, 2, MPI_INT, source, TAG_RESULT, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);

        int r_start_y = result_meta[0];
        int r_h = result_meta[1];

        MPI_Recv(output + r_start_y * width, r_h * width * sizeof(Pixel),
                 MPI_BYTE, source, TAG_RESULT, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
      }
    }

    free(img->data);
    img->data = output;
  }

  // ---------------------------------------------------------
  // WORKER LOGIC
  // ---------------------------------------------------------
  else {
    while (1) {
      // 1. Send Request
      int dummy = 0;
      MPI_Send(&dummy, 1, MPI_INT, 0, TAG_REQUEST, MPI_COMM_WORLD);

      // 2. Wait for Task or Terminate
      MPI_Status status;
      MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

      if (status.MPI_TAG == TAG_TERMINATE) {
        MPI_Recv(NULL, 0, MPI_INT, 0, TAG_TERMINATE, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);
        break;
      } else if (status.MPI_TAG == TAG_TASK) {
        int task_meta[3];
        MPI_Recv(task_meta, 3, MPI_INT, 0, TAG_TASK, MPI_COMM_WORLD,
                 MPI_STATUS_IGNORE);

        int global_start_y = task_meta[0];
        int output_h = task_meta[1];
        int input_h = task_meta[2];

        Pixel *input_buf = (Pixel *)malloc(input_h * width * sizeof(Pixel));
        if (!input_buf)
          return ERR_MEM_ALLOC;
        MPI_Recv(input_buf, input_h * width * sizeof(Pixel), MPI_BYTE, 0,
                 TAG_TASK, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        Pixel *output_buf = (Pixel *)malloc(output_h * width * sizeof(Pixel));
        if (!output_buf) {
          free(input_buf);
          return ERR_MEM_ALLOC;
        }

        int half_k = halo_size;

#pragma omp parallel for collapse(2) schedule(static)
        for (int y = 0; y < output_h; y++) {
          for (int x = 0; x < width; x++) {
            double r_acc = 0, g_acc = 0, b_acc = 0;

            for (int ky = 0; ky < k_size; ky++) {
              for (int kx = 0; kx < k_size; kx++) {
                int py = y + halo_size + ky - half_k;
                int px = x + kx - half_k;

                // Clamp X
                if (px < 0)
                  px = 0;
                if (px >= width)
                  px = width - 1;

                Pixel p = input_buf[py * width + px];
                double k_val = local_kernel_data[ky * k_size + kx];

                r_acc += p.r * k_val;
                g_acc += p.g * k_val;
                b_acc += p.b * k_val;
              }
            }

            Pixel out_p;
            clamp_pixel(&out_p, r_acc, g_acc, b_acc);
            output_buf[y * width + x] = out_p;
          }
        }

        free(input_buf);

        // 4. Send Result
        int result_meta[2] = {global_start_y, output_h};
        MPI_Send(result_meta, 2, MPI_INT, 0, TAG_RESULT, MPI_COMM_WORLD);
        MPI_Send(output_buf, output_h * width * sizeof(Pixel), MPI_BYTE, 0,
                 TAG_RESULT, MPI_COMM_WORLD);

        free(output_buf);
      }
    }
  }

  // Cleanup
  if (rank != 0) {
    free(local_kernel_data);
  }

  double end_time = MPI_Wtime();
  if (elapsed_time != NULL)
    *elapsed_time = end_time - start_time;

  return SUCCESS;
}
