#include "image/bmp_io.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define HEADER_SIZE 54
#define HEADER_FILE_SIZE_OFFSET 2
#define HEADER_WIDTH_OFFSET 18
#define HEADER_HEIGHT_OFFSET 22
#define HEADER_BITS_PER_PIXEL_OFFSET 28
#define BITS_PER_PIXEL 24

Pixel *alloc_pixel(int width, int height) {
  Pixel *data = (Pixel *)malloc(width * height * sizeof(Pixel));

  if (!data) {
    fprintf(stderr, "Error: Memory allocation failed for pixels\n");
    return NULL;
  }

  return data;
}

Image *alloc_image(Pixel *data, int width, int height) {
  Image *img = (Image *)malloc(sizeof(Image));

  if (!img) {
    fprintf(stderr, "Error: Memory allocation failed for image\n");
    return NULL;
  }

  img->width = width;
  img->height = height;
  img->data = data;
  return img;
}

/* Read BMP file, build and return Image struct */
app_error read_BMP(Image **img, const char *filename) {
  FILE *f = fopen(filename, "rb");
  if (!f) {
    fprintf(stderr, "Error: Could not open file %s: %s\n", filename,
            strerror(errno));
    return ERR_FILE_OPEN;
  }

  unsigned char header[HEADER_SIZE];
  if (fread(header, sizeof(unsigned char), HEADER_SIZE, f) != HEADER_SIZE) {
    fprintf(stderr, "Error: Invalid BMP header\n");
    fclose(f);
    return ERR_BMP_HEADER;
  }

  if (header[0] != 'B' || header[1] != 'M') {
    fprintf(stderr, "Error: Not a valid BMP file\n");
    fclose(f);
    return ERR_BMP_HEADER;
  }

  int width = *(int *)&header[HEADER_WIDTH_OFFSET];
  int height = *(int *)&header[HEADER_HEIGHT_OFFSET];
  int bitsPerPixel = *(short *)&header[HEADER_BITS_PER_PIXEL_OFFSET];

  if (bitsPerPixel != BITS_PER_PIXEL) {
    fprintf(stderr, "Error: Only 24-bit BMPs are supported\n");
    fclose(f);
    return ERR_BMP_HEADER;
  }

  int row_padded = (width * 3 + 3) & (~3);
  unsigned char *row = (unsigned char *)malloc(row_padded);
  Pixel *data = alloc_pixel(width, height);

  if (!data || !row) {
    fprintf(stderr, "Error: Memory allocation failed\n");
    if (data)
      free(data);
    if (row)
      free(row);
    fclose(f);
    return ERR_MEM_ALLOC;
  }

  for (int y = 0; y < height; y++) {
    fread(row, sizeof(unsigned char), row_padded, f);
    for (int x = 0; x < width; x++) {
      data[(height - 1 - y) * width + x].b = row[x * 3];
      data[(height - 1 - y) * width + x].g = row[x * 3 + 1];
      data[(height - 1 - y) * width + x].r = row[x * 3 + 2];
    }
  }

  free(row);
  fclose(f);

  *img = alloc_image(data, width, height);
  if (!*img) {
    free(data);
    return ERR_MEM_ALLOC;
  }

  return SUCCESS;
}

app_error copy_image(const Image *src, Image **dest) {
  if (!src || !dest) {
    return ERR_INVALID_ARGS;
  }

  // Allocate new pixels
  Pixel *new_data = alloc_pixel(src->width, src->height);
  if (!new_data) {
    return ERR_MEM_ALLOC;
  }

  // Copy pixel data
  memcpy(new_data, src->data, src->width * src->height * sizeof(Pixel));

  // Allocate new image struct
  *dest = alloc_image(new_data, src->width, src->height);
  if (!*dest) {
    free(new_data);
    return ERR_MEM_ALLOC;
  }

  return SUCCESS;
}

/* Save Image in file in BMP format */
app_error save_BMP(const Image *img, const char *filename) {
  FILE *f = fopen(filename, "wb");
  if (!f) {
    fprintf(stderr, "Error: Could not create file %s\n", filename);
    return ERR_FILE_OPEN;
  }

  int width = img->width;
  int height = img->height;
  int row_padded = (width * 3 + 3) & (~3);
  int fileSize = HEADER_SIZE + row_padded * height;

  unsigned char header[HEADER_SIZE] = {
      'B', 'M',       // Signature
      0,   0,   0, 0, // File size
      0,   0,   0, 0, // Reserved
      54,  0,   0, 0, // Data offset
      40,  0,   0, 0, // Header size
      0,   0,   0, 0, // Width
      0,   0,   0, 0, // Height
      1,   0,         // Planes
      24,  0,         // Bits per pixel
      0,   0,   0, 0, // Compression (none)
      0,   0,   0, 0, // Image size (can be 0 for no compression)
      0,   0,   0, 0, // X pixels per meter
      0,   0,   0, 0, // Y pixels per meter
      0,   0,   0, 0, // Total colors
      0,   0,   0, 0  // Important colors
  };

  // Fill in width, height, and file size
  *(int *)&header[HEADER_FILE_SIZE_OFFSET] = fileSize;
  *(int *)&header[HEADER_WIDTH_OFFSET] = width;
  *(int *)&header[HEADER_HEIGHT_OFFSET] = height;

  fwrite(header, sizeof(unsigned char), HEADER_SIZE, f);

  unsigned char *row = (unsigned char *)calloc(1, row_padded);
  if (!row) {
    fprintf(stderr, "Error: Memory allocation failed\n");
    fclose(f);
    return ERR_MEM_ALLOC;
  }

  // Write pixel data bottom-to-top
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      Pixel pixel = img->data[(height - 1 - y) * width + x];
      row[x * 3] = pixel.b;
      row[x * 3 + 1] = pixel.g;
      row[x * 3 + 2] = pixel.r;
    }
    fwrite(row, sizeof(unsigned char), row_padded, f);
  }

  free(row);
  fclose(f);
  return SUCCESS;
}

void free_BMP(Image *img) {
  free(img->data);
  free(img);
}

void print_BMP_header(const Image *img, FILE *fp) {
  fprintf(fp, "Image header is width=%d  and height=%d \n", img->width,
          img->height);
}

void print_pixel(const Pixel pixel, FILE *fp) {
  fprintf(fp, "(%u, %u, %u) ", pixel.r, pixel.g, pixel.b);
}

void print_BMP_pixel(const Image *img, int x, int y, FILE *fp) {
  print_pixel(img->data[y * img->width + x], fp);
}

void print_BMP_pixels(const Image *img, FILE *fp) {
  for (int y = 0; y < img->height; y++) {
    for (int x = 0; x < img->width; x++) {
      print_BMP_pixel(img, x, y, fp);
    }
    fprintf(fp, "\n");
  }
}
