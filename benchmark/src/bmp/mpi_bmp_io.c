#include "mpi_bmp_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

app_error mpi_read_BMP_chunk(Image **img, const char *filename, int start_row,
                             int num_rows, int *total_width,
                             int *total_height) {
  MPI_File fh;
  int err;
  MPI_Status status;
  unsigned char header[54];

  // Open file
  err = MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_RDONLY, MPI_INFO_NULL,
                      &fh);
  if (err != MPI_SUCCESS) {
    return ERR_FILE_OPEN;
  }

  // Read header (everyone reads it, or just one broadcasts - everyone reading
  // is fine for small header) Using MPI_File_read_at to read header from
  // beginning
  err = MPI_File_read_at(fh, 0, header, 54, MPI_BYTE, &status);
  if (err != MPI_SUCCESS) {
    MPI_File_close(&fh);
    return ERR_BMP_HEADER;
  }

  if (header[0] != 'B' || header[1] != 'M') {
    MPI_File_close(&fh);
    return ERR_BMP_HEADER;
  }

  int width = *(int *)&header[18];
  int height = *(int *)&header[22];
  int bitsPerPixel = *(short *)&header[28];

  if (total_width)
    *total_width = width;
  if (total_height)
    *total_height = height;

  if (bitsPerPixel != 24) {
    MPI_File_close(&fh);
    return ERR_BMP_HEADER;
  }

  // Allocate local image chunk
  Pixel *data = alloc_pixel(width, num_rows);
  if (!data) {
    MPI_File_close(&fh);
    return ERR_MEM_ALLOC;
  }

  int row_padded = (width * 3 + 3) & (~3);

  // Calculate File Offset
  // Memory Range: [start_row, start_row + num_rows - 1] (Top-Down)
  // File Rows: [height - 1 - (start_row + num_rows - 1), height - 1 -
  // start_row]
  //            = [height - start_row - num_rows, height - start_row - 1]

  // Ensure we don't read out of bounds (caller should handle, but sanity check)
  // Actually, caller might request halo rows that are "outside" (e.g. -1).
  // But we can only read from file what exists.
  // If start_row < 0, we can't read those.
  // So this function assumes start_row and num_rows map to VALID file rows.

  // Simplest: The file contains rows 0 to height-1.
  // We want Memory Rows start_row...start_row+num_rows.
  // These correspond to File Rows:
  // First Memory Row (start_row) -> File Row: height - 1 - start_row
  // Last Memory Row (start_row + num_rows - 1) -> File Row: height - 1 -
  // (start_row + num_rows - 1)

  // Since File is Bottom-Up, the "First Memory Row" is higher in the file
  // (larger offset). The "Last Memory Row" is lower in the file (smaller
  // offset). So we read from the offset of the Last Memory Row.

  int last_mem_row = start_row + num_rows - 1;
  int first_file_row_to_read = height - 1 - last_mem_row;

  // Offset in bytes
  MPI_Offset file_offset = 54 + (MPI_Offset)first_file_row_to_read * row_padded;

  // Size to read
  int bytes_to_read = num_rows * row_padded;

  unsigned char *buffer = (unsigned char *)malloc(bytes_to_read);
  if (!buffer) {
    free(data);
    MPI_File_close(&fh);
    return ERR_MEM_ALLOC;
  }

  err = MPI_File_read_at(fh, file_offset, buffer, bytes_to_read, MPI_BYTE,
                         &status);
  if (err != MPI_SUCCESS) {
    free(buffer);
    free(data);
    MPI_File_close(&fh);
    return ERR_FILE_OPEN; // Or read error
  }

  // Now fill data from buffer
  // Buffer starts with 'first_file_row_to_read' which corresponds to
  // 'last_mem_row' Buffer contains rows in ascending file order (Bottom-Up
  // relative to image) So Buffer[0..row_padded] is Memory Row 'last_mem_row'.
  // Buffer[row_padded..] is Memory Row 'last_mem_row - 1'.
  // ...
  // Buffer[end] is Memory Row 'start_row'.

  for (int i = 0; i < num_rows; i++) {
    // i goes 0 (first row in buffer) to num_rows-1
    // This corresponds to Memory Row: last_mem_row - i?
    // Let's trace:
    // Buffer Row 0 = File Row X.
    // File Row X corresponds to Memory Row (height - 1 - X).
    // X = first_file_row_to_read = height - last_mem_row - 1 -> Memory Row =
    // height - 1 - (height - last_mem_row - 1) = last_mem_row. So Buffer Row 0
    // goes to last_mem_row in 'data'. 'data' is size 'num_rows'. Its indices
    // are 0..(num_rows-1). Indices relative to 'start_row': data[k] corresponds
    // to Global Row 'start_row + k'. So we want to fill data[last_mem_row -
    // start_row]. last_mem_row - start_row = (start_row + num_rows - 1) -
    // start_row = num_rows - 1.

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

  if (rank == 0) {
    int fileSize = 54 + row_padded * total_height;
    unsigned char header[54] = {'B', 'M', 0, 0, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0,
                                40,  0,   0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 1, 0,
                                24,  0,   0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0,
                                0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0,  0};

    *(int *)&header[2] = fileSize;
    *(int *)&header[18] = total_width;
    *(int *)&header[22] = total_height;

    MPI_File_write_at(fh, 0, header, 54, MPI_BYTE, &status);
  }

  // Wait for header? Not strictly necessary if writing to different offsets,
  // but good practice. MPI_Barrier(MPI_COMM_WORLD);

  // Calculate Offset
  // We are writing 'img' which starts at global 'start_row'.
  // num_rows = img->height.
  // Similar logic to read:
  // Memory Row k (0..h-1) -> Global Row start_row + k.
  // Global Row R -> File Row height - 1 - R.

  // This implies non-contiguous writes if we iterate k=0..h-1.
  // File Row for k=0 is (Height-1 - start_row).
  // File Row for k=1 is (Height-1 - start_row - 1). (Lower in file).

  // So we can prepare a buffer for the whole chunk and write it.
  // But the buffer must be ordered by FILE rows (Bottom-Up).
  // File Rows range: [Height - 1 - (start_row + h - 1), Height - 1 -
  // start_row]. Smallest File Row index: Height - start_row - h. Largest File
  // Row: Height - start_row - 1.

  int num_local_rows = img->height;
  int bytes_to_write = num_local_rows * row_padded;
  unsigned char *buffer = (unsigned char *)calloc(1, bytes_to_write);
  if (!buffer) {
    MPI_File_close(&fh);
    return ERR_MEM_ALLOC;
  }

  // Fill buffer
  // Buffer runs from Smallest File Row to Largest.
  // Buffer[0...row_padded] = File Row (Height - start_row - h).
  // This File Row corresponds to Memory Row: Height - 1 - (Height - start_row -
  // h) = start_row + h - 1. So Buffer[0] gets Data[h-1]. Buffer[last] gets
  // Data[0].

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
      54 + (MPI_Offset)first_file_row_to_write * row_padded;

  err = MPI_File_write_at(fh, file_offset, buffer, bytes_to_write, MPI_BYTE,
                          &status);

  free(buffer);
  MPI_File_close(&fh);

  if (err != MPI_SUCCESS)
    return ERR_FILE_OPEN; // Write error
  return SUCCESS;
}
