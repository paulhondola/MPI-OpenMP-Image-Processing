#ifndef MPI_BMP_IO_H
#define MPI_BMP_IO_H

#include "bmp_io.h"
#include <mpi.h>

/**
 * @brief Reads a chunk of a BMP file using MPI I/O.
 *
 * @param img Pointer to Image pointer. Will be allocated.
 * @param filename File to read.
 * @param start_row Starting row index (Top-Down, 0 is top) to read (INCLUSIVE).
 * @param num_rows Number of rows to read.
 * @param total_width Output parameter: width of the full image.
 * @param total_height Output parameter: height of the full image.
 * @return app_error
 */
app_error mpi_read_BMP_chunk(Image **img, const char *filename, int start_row,
                             int num_rows, int *total_width, int *total_height);

/**
 * @brief Writes a chunk of a BMP file using MPI I/O.
 *
 * @param img Pointer to the chunk Image.
 * @param filename File to write to.
 * @param start_row The global starting row index (Top-Down) where this chunk
 * belongs.
 * @param total_width The width of the full image.
 * @param total_height The height of the full image.
 * @return app_error
 */
app_error mpi_write_BMP_chunk(const Image *img, const char *filename,
                              int start_row, int total_width, int total_height);

#endif // MPI_BMP_IO_H
