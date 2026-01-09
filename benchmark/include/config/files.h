#ifndef __FILES_H__
#define __FILES_H__

// Constants for benchmarking
#define CONVOLUTION_MODES 5
#define BENCHMARK_FILES 3
#define KERNEL_TYPES 7

// Stores the elapsed times for each mode, file, and kernel
extern double benchmark_data[CONVOLUTION_MODES][BENCHMARK_FILES][KERNEL_TYPES];

// Constants for file paths and names
extern const char *files[];

// Directories
extern const char *IMAGES_FOLDER;
extern const char *BASE_FOLDER;
extern const char *SERIAL_FOLDER;
extern const char *MULTITHREADED_FOLDER;
extern const char *DISTRIBUTED_FOLDER;
extern const char *SHARED_FOLDER;
extern const char *TASK_POOL_FOLDER;
extern const char *IMPLEMENTATION_FOLDERS[];

// CSV Files
extern const char *SINGLE_RUN_CSV_FILE;
extern const char *MULTI_RUN_CSV_FILE;
extern const char *SERIAL_CSV_FILE;
extern const char *MULTITHREADED_CSV_FILE;
extern const char *DISTRIBUTED_CSV_FILE;
extern const char *SHARED_CSV_FILE;
extern const char *TASK_POOL_CSV_FILE;

// CSV Headers
extern const char *SINGLE_RUN_CSV_HEADER;
extern const char *MULTI_RUN_CSV_HEADER;

// CSV Headers for speedup
extern const char *SPEEDUP_CSV_FILE;
extern const char *SPEEDUP_CSV_HEADER;

#endif
