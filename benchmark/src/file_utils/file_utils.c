#include "file_utils.h"
#include "../config/files.h"
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>

app_error create_directory(const char *path) {
  if (mkdir(path, 0777) == -1) {
    if (errno != EEXIST) {
      perror("mkdir");
      return ERR_DIR_CREATE;
    }
  }
  return SUCCESS;
}

app_error init_benchmark_csv(const char *filename, const char *header) {
  FILE *fp = fopen(filename, "a+");
  if (fp == NULL) {
    perror("Error opening CSV file");
    return ERR_FILE_OPEN;
  }

  // Check if file is empty
  if (fseek(fp, 0, SEEK_END) != 0) {
    perror("Error seeking to end of CSV file");
    fclose(fp);
    return ERR_FILE_IO;
  }
  long size = ftell(fp);
  if (size == -1L) {
    perror("Error telling position in CSV file");
    fclose(fp);
    return ERR_FILE_IO;
  }

  // If file is empty, write csv header
  if (size == 0) {
    fprintf(fp, "%s\n", header);
  }

  fclose(fp);
  return SUCCESS;
}

app_error append_benchmark_result(const char *filename, int pixel_count,
                                  int kernel_size, int clusters, int threads,
                                  double serial_time, double multithreaded_time,
                                  double distributed_time, double shared_time,
                                  double task_pool_time) {
  FILE *fp = fopen(filename, "a");
  if (fp == NULL) {
    perror("Error opening CSV file for appending");
    return ERR_FILE_OPEN;
  }

  fprintf(fp, "%d,%d,%d,%d,%.6f,%.6f,%.6f,%.6f,%.6f\n", pixel_count,
          kernel_size, clusters, threads, serial_time, multithreaded_time,
          distributed_time, shared_time, task_pool_time);

  fclose(fp);
  return SUCCESS;
}

app_error append_single_benchmark_result(const char *filename, int pixel_count,
                                         int kernel_size, int clusters,
                                         int threads, double time) {
  FILE *fp = fopen(filename, "a");
  if (fp == NULL) {
    perror("Error opening CSV file for appending");
    return ERR_FILE_OPEN;
  }

  fprintf(fp, "%d,%d,%d,%d,%.6f\n", pixel_count, kernel_size, clusters, threads,
          time);

  fclose(fp);
  return SUCCESS;
}
