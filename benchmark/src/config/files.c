#include "files.h"

// Fallback in case the compiler flag isn't set
#ifndef PROJECT_ROOT
#define PROJECT_ROOT ""
#endif

// CSV Header for the single-run data file
const char *SINGLE_RUN_CSV_HEADER =
    "Pixel Count,Kernel Size,Clusters,Threads,Time";

#define _DATA_FOLDER PROJECT_ROOT "data/chronos"
const char *SERIAL_CSV_FILE = _DATA_FOLDER "/serial_data.csv";
const char *MULTITHREADED_CSV_FILE = _DATA_FOLDER "/multithreaded_data.csv";
const char *DISTRIBUTED_CSV_FILE = _DATA_FOLDER "/distributed_data.csv";
const char *SHARED_CSV_FILE = _DATA_FOLDER "/shared_data.csv";
const char *TASK_POOL_CSV_FILE = _DATA_FOLDER "/task_pool_data.csv";

// CSV Header for the multi-run data file
const char *MULTI_RUN_CSV_FILE = _DATA_FOLDER "/time_data.csv";
const char *MULTI_RUN_CSV_HEADER =
    "Pixel Count,Kernel Size,Clusters,Threads,Serial "
    "Time,Multithreaded "
    "Time,Distributed Time,Shared Time,Task Pool Time";

// CSV Header for the speedup data file
const char *SPEEDUP_CSV_FILE = _DATA_FOLDER "/speedups_data.csv";
const char *SPEEDUP_CSV_HEADER =
    "Pixel Count,Kernel Size,Clusters,Threads,Serial "
    "Speedup,Multithreaded "
    "Speedup,Distributed Speedup,Shared Speedup,Task Pool Speedup";

// Image directories
#define _IMAGES_FOLDER PROJECT_ROOT "images"
#define _BASE_FOLDER "base"
#define _SERIAL_FOLDER "serial"
#define _MULTITHREADED_FOLDER "multithreaded"
#define _DISTRIBUTED_FOLDER "distributed"
#define _SHARED_FOLDER "shared"
#define _TASK_POOL_FOLDER "task_pool"

const char *IMAGES_FOLDER = _IMAGES_FOLDER;
const char *BASE_FOLDER = _BASE_FOLDER;
const char *SERIAL_FOLDER = _SERIAL_FOLDER;
const char *MULTITHREADED_FOLDER = _MULTITHREADED_FOLDER;
const char *DISTRIBUTED_FOLDER = _DISTRIBUTED_FOLDER;
const char *SHARED_FOLDER = _SHARED_FOLDER;
const char *TASK_POOL_FOLDER = _TASK_POOL_FOLDER;
const char *IMPLEMENTATION_FOLDERS[] = {_SERIAL_FOLDER, _MULTITHREADED_FOLDER,
                                        _DISTRIBUTED_FOLDER, _SHARED_FOLDER,
                                        _TASK_POOL_FOLDER};

// Input files (located in images/base)
const char *files[] = {"Large.bmp", "XL.bmp", "XXL.bmp"};

// Elapsed time data for each convolution mode, file, and kernel type
double benchmark_data[CONVOLUTION_MODES][BENCHMARK_FILES][KERNEL_TYPES];
