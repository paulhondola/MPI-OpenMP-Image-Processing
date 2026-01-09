#include "image/mpi_bmp_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HEADER_SIZE 54
#define HEADER_FILE_SIZE_OFFSET 2
#define HEADER_WIDTH_OFFSET 18
#define HEADER_HEIGHT_OFFSET 22
#define HEADER_BITS_PER_PIXEL_OFFSET 28
#define BITS_PER_PIXEL 24

app_error mpi_read_BMP_chunk(Image **img, const char *filename, int start_row,
                             int num_rows, int *total_width,
                             int *total_height) {
  MPI_File fh;
  int err;
  MPI_Status status;
  unsigned char header[HEADER_SIZE];

  // Open file
  err = MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_RDONLY, MPI_INFO_NULL,
                      &fh);
  if (err != MPI_SUCCESS) {
    return ERR_FILE_OPEN;
  }

  // Read header
  err = MPI_File_read_at(fh, 0, header, HEADER_SIZE, MPI_BYTE, &status);
  if (err != MPI_SUCCESS) {
    MPI_File_close(&fh);
    return ERR_BMP_HEADER;
  }

  // Validate header, if not BMP, return error
  if (header[0] != 'B' || header[1] != 'M') {
    MPI_File_close(&fh);
    return ERR_BMP_HEADER;
  }

  // Extract image dimensions and bits per pixel
  int width = *(int *)&header[HEADER_WIDTH_OFFSET];
  int height = *(int *)&header[HEADER_HEIGHT_OFFSET];
  int bitsPerPixel = *(short *)&header[HEADER_BITS_PER_PIXEL_OFFSET];

  // Return image dimensions
  if (total_width)
    *total_width = width;
  if (total_height)
    *total_height = height;

  // Validate bits per pixel
  if (bitsPerPixel != BITS_PER_PIXEL) {
    MPI_File_close(&fh);
    return ERR_BMP_HEADER;
  }

  // Allocate local image chunk
  Pixel *data = alloc_pixel(width, num_rows);
  if (!data) {
    MPI_File_close(&fh);
    return ERR_MEM_ALLOC;
  }

  // Calculate row padding and file offset
  int row_padded = (width * 3 + 3) & (~3);
  int last_mem_row = start_row + num_rows - 1;
  int first_file_row_to_read = height - 1 - last_mem_row;

  // Offset in bytes
  MPI_Offset file_offset =
      HEADER_SIZE + (MPI_Offset)first_file_row_to_read * row_padded;

  // Size to read
  int bytes_to_read = num_rows * row_padded;

  // Allocate buffer for reading
  unsigned char *buffer = (unsigned char *)malloc(bytes_to_read);
  if (!buffer) {
    free(data);
    MPI_File_close(&fh);
    return ERR_MEM_ALLOC;
  }

  // Read chunk from file
  err = MPI_File_read_at(fh, file_offset, buffer, bytes_to_read, MPI_BYTE,
                         &status);
  if (err != MPI_SUCCESS) {
    free(buffer);
    free(data);
    MPI_File_close(&fh);
    return ERR_FILE_OPEN;
  }

  // Fill data from buffer
  for (int i = 0; i < num_rows; i++) {
    int target_data_row = (num_rows - 1) - i;
    unsigned char *row_ptr = buffer + i * row_padded;

    for (int x = 0; x < width; x++) {
      data[target_data_row * width + x].b = row_ptr[x * 3];
      data[target_data_row * width + x].g = row_ptr[x * 3 + 1];
      data[target_data_row * width + x].r = row_ptr[x * 3 + 2];
    }
  }

  free(buffer);
  MPI_File_close(&fh);

  *img = alloc_image(data, width, num_rows);
  if (!*img) {
    free(data);
    return ERR_MEM_ALLOC;
  }

  return SUCCESS;
}

app_error mpi_write_BMP_chunk(const Image *img, const char *filename,
                              int start_row, int total_width,
                              int total_height) {
  MPI_File fh;
  int err;
  MPI_Status status;

  // Open file - Create if not exists
  // Note: MPI_MODE_CREATE | MPI_MODE_WRONLY
  err = MPI_File_open(MPI_COMM_WORLD, filename,
                      MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &fh);
  if (err != MPI_SUCCESS) {
    return ERR_FILE_OPEN;
  }

  // Rank 0 writes header
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  int row_padded = (total_width * 3 + 3) & (~3);

  // Write header, only the root process does this
  if (rank == 0) {
    int fileSize = 54 + row_padded * total_height;
    unsigned char header[HEADER_SIZE] = {
        'B', 'M', 0, 0, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0, 40, 0, 0, 0,
        0,   0,   0, 0, 0, 0, 0, 0, 1, 0, 24, 0, 0, 0, 0,  0, 0, 0,
        0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0};

    *(int *)&header[HEADER_FILE_SIZE_OFFSET] = fileSize;
    *(int *)&header[HEADER_WIDTH_OFFSET] = total_width;
    *(int *)&header[HEADER_HEIGHT_OFFSET] = total_height;

    MPI_File_write_at(fh, 0, header, HEADER_SIZE, MPI_BYTE, &status);
  }

  // Write image data
  int num_local_rows = img->height;
  int bytes_to_write = num_local_rows * row_padded;
  unsigned char *buffer = (unsigned char *)calloc(1, bytes_to_write);
  if (!buffer) {
    MPI_File_close(&fh);
    return ERR_MEM_ALLOC;
  }

  for (int i = 0; i < num_local_rows; i++) {
    // Buffer index i corresponds to Data row (h - 1 - i).

    int data_row_idx = (num_local_rows - 1) - i;
    unsigned char *row_ptr = buffer + i * row_padded;

    for (int x = 0; x < img->width; x++) {
      Pixel pixel = img->data[data_row_idx * img->width + x];
      row_ptr[x * 3] = pixel.b;
      row_ptr[x * 3 + 1] = pixel.g;
      row_ptr[x * 3 + 2] = pixel.r;
    }
  }

  // Calculate write offset
  // Offset of Smallest File Row
  int first_file_row_to_write =
      total_height - 1 - (start_row + num_local_rows - 1);
  MPI_Offset file_offset =
      HEADER_SIZE + (MPI_Offset)first_file_row_to_write * row_padded;

  err = MPI_File_write_at(fh, file_offset, buffer, bytes_to_write, MPI_BYTE,
                          &status);

  free(buffer);
  MPI_File_close(&fh);

  if (err != MPI_SUCCESS)
    return ERR_FILE_OPEN;

  return SUCCESS;
}
