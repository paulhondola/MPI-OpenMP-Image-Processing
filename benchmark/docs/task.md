Operations such as blurring, sharpening, embossing, and edge detection can be performed by computing each pixel in the output image as a function of nearby pixels (including the pixel itself) in the input image. This function is represented as a small matrix called a kernel, which is applied to the image through a convolution operation. For RGB images, the convolution is carried out separately on each of the three color channels.

Implement  an image transformer  that can apply a selected transformation on a  image.

The image transformer will take as arguments: the operation, input file, and output file.

The operations can be any of:  RIDGE,  EDGE,  SHARPEN, BOXBLUR, GAUSSBLUR3, GAUSSBLUR5, UNSHARP5.  All  transformations are realised by the same algorithm of convolution - but the size of the kernel and the kernel varies.  See https://en.wikipedia.org/wiki/Kernel_(image_processing).

Assume that the image transformer will be deployed on a cluster of p workstations, every workstation having c cores.

At deployment time, the user will specify if there is a shared filesystem: accessible to all processes. If there is no shared file system, one process will do the reading of the BMP file and send chunks to other processes.  If there is a shared filesystem, the transformer should avoid sending  image chunks via messages. 

Chose an appropriate data parallel architecture for the image transformer, exploiting both message passing and shared memory parallelism by combining MPI and OpenMP.

Make sure that the parallel version and serial version produce the same results, by comparing the output files.

Measure the speedup obtained by parallelization.

Assume that the image transformer accepts only image files that are 24-bit BMP files (every pixel is represented by 3 bytes corresponding to Red Green Blue).  Archives with samples of large and extralarge BMP files for testing:

Large.zip

XL.zip

XXL.zip

In bmp_io.c  you are given  code that reads BMP files  and  constructs their representation as a grid of pixels in memory, and also saves as BMP file the in memory representation of an image.  Also a short example program of image transformation  is given: it loads a BMP, adds a black stripe, and saves the resulted image.

Bonus requirements: (1 bonus point)

Implement a second version of the image transformer using a producer/worker work-pool.
One MPI process must incrementally read the BMP file in chunks  and distribute these chunks to worker processes on demand. (Attention: include the halo into the distributed chunks!)  Workers form a dynamic work-pool: each worker requests a chunk, processes it and returns the result. The producer assembles the processed chunks into the final output image and signals workers to terminate when all chunks are done.

Make sure  that this version produces correct results by comparing the output file with output produced by the serial version.

Determine an optimal chunksize (experimental, for different number of processes and image size).

Measure the Speedup, compare with  the serial version and with  the data parallel  version.
