#include "../../include/config/file_utils.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TEST_FILE "test_output.csv"

int main(void) {
  // Setup inputs
  int width = 1920;
  int height = 1080;
  int kernel_size = 3;
  int clusters = 4;
  int threads = 2; // 4 clusters * 8 threads
  double serial = 10.0;
  double multi = 2.5;
  double dist = 1.0;
  double shared = 1.2;
  double task = 1.1;

  // Expected Output Calculation
  // Header: Width * Height,Kernel Size,Serial Time,Threads,Multithreaded
  // Time,Clusters,Threads,Distributed Time,Shared Time,Task Pool Time Local
  // Threads = total_threads / clusters = 32 / 4 = 8 Format: "%d *
  // %d,%d,%.6f,%d,%.6f,%d,%d,%.6f,%.6f,%.6f\n"

  char expected_line[512];
  snprintf(
      expected_line, sizeof(expected_line),
      "1920 * 1080,3,10.000000,8,2.500000,4,2,1.000000,1.200000,1.100000\n");

  // Clean up previous run
  remove(TEST_FILE);

  // Run Function
  app_error err =
      append_benchmark_result(TEST_FILE, width, height, kernel_size, clusters,
                              threads, serial, multi, dist, shared, task);

  if (err != SUCCESS) {
    fprintf(stderr, "Function failed with error code: %d\n", err);
    return 1;
  }

  // Verify Output
  FILE *fp = fopen(TEST_FILE, "r");
  if (!fp) {
    perror("Failed to open test file");
    return 1;
  }

  char line[512];
  if (!fgets(line, sizeof(line), fp)) {
    fprintf(stderr, "Failed to read line from test file\n");
    fclose(fp);
    return 1;
  }
  fclose(fp);

  // Remove test file
  remove(TEST_FILE);

  // Compare
  if (strcmp(line, expected_line) != 0) {
    fprintf(stderr, "Test FAILED!\n");
    fprintf(stderr, "Expected: %s", expected_line);
    fprintf(stderr, "Actual:   %s", line);
    return 1;
  }

  printf("Test PASSED\n");
  return 0;
}
