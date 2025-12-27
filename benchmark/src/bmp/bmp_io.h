#ifndef __BMP_IO_H__
#define __BMP_IO_H__

#include "../errors/errors.h"
#include <stdio.h>

/* Data structures for representing BMP images in memory */

/**
 * Represents a single RGB pixel.
 */
typedef struct {
  unsigned char r, g, b;
} Pixel; // one RGB point

/**
 * Represents a BMP image in memory.
 * Contains dimensions and pixel data.
 */
typedef struct {
  int width;
  int height;
  Pixel *data;
} Image; // a BMP image as an array of RGB points

/* Read BMP file, build and return Image struct via pointer */
/**
 * Reads a BMP file and creates an Image structure.
 * Allocates memory for the Image and its pixel data.
 * @param filename Path to the input BMP file
 * @param img Pointer to text_image pointer to store the result
 * @return app_error code:
 *         - SUCCESS: File read successfully
 *         - ERR_FILE_OPEN: Could not open file
 *         - ERR_BMP_HEADER: Invalid or unsupported BMP header
 *         - ERR_MEM_ALLOC: Memory allocation failed
 */
app_error read_BMP(Image **img, const char *filename);

/**
 * Creates a deep copy of an Image structure.
 * @param src Pointer to the source Image
 * @param dest Pointer to a pointer that will hold the new Image
 * @return app_error code
 */
app_error copy_image(const Image *src, Image **dest);

/**
 * Saves an Image structure to a BMP file.
 * @param filename Path to the output BMP file
 * @param img Pointer to the Image structure to save
 * @return app_error code:
 *         - SUCCESS: File saved successfully
 *         - ERR_FILE_OPEN: Could not create/open file
 *         - ERR_MEM_ALLOC: Memory allocation failed during save
 */
app_error save_BMP(const Image *img, const char *filename);

/* Allocators */
/**
 * Allocates memory for an array of pixels.
 * @param width Image width
 * @param height Image height
 * @return Pointer to allocated Pixel array or NULL on failure
 */
Pixel *alloc_pixel(int width, int height);

/**
 * Allocates memory for an Image structure.
 * @param width Image width
 * @param height Image height
 * @param data Optional pointer to existing pixel data (can be NULL)
 * @return Pointer to allocated Image structure or NULL on failure
 */
Image *alloc_image(Pixel *data, int width, int height);

/* Free memory allocated for Image */
/**
 * Frees memory allocated for an Image structure and its pixel data.
 * @param img Pointer to the Image structure to free
 */
void free_BMP(Image *img);

/* Debug */
/**
 * Prints BMP header information to the specified file stream.
 * @param img Pointer to the Image structure
 * @param fp File stream to write to
 */
void print_BMP_header(const Image *img, FILE *fp);

/**
 * Prints pixel data to the specified file stream (for debugging).
 * @param img Pointer to the Image structure
 * @param fp File stream to write to
 */
void print_BMP_pixels(const Image *img, FILE *fp);

#endif
