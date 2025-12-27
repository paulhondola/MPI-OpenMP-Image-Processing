# MPI & OpenMP Distributed and Multithreaded Image Processing

This project implements efficient image processing kernels using a hybrid parallelization approach. The core task is to apply various convolution kernels (such as Edge Detection, Gaussian Blur, and Sharpening) to large BMP images.

## Project Overview

The primary goal of this project is to demonstrate and benchmark high-performance computing techniques in C. It compares the performance of:

1.  **Serial Execution**: Baseline implementation on a single core.
2.  **Simple Multithreaded**: Shared-memory parallelism utilizing OpenMP threads.
3.  **Hybrid (MPI + OpenMP)**: Combining both approaches in three distinct configurations:
    - **Message Passing**: Distributed-memory approach.
    - **Shared Memory**: Parallelism via shared memory access.
    - **Task Pool**: Dynamic task scheduling for load balancing.

### The Task: Convolution

Convolution is a mathematical operation fundamental to many image processing techniques. It involves sliding a small matrix (the kernel) over the image pixels to calculate new pixel values based on their neighbors. This operation is computationally intensive and highly parallelizable, making it an excellent candidate for performance optimization studies.

This project implements several standard kernels:
*   **Ridge Detection**: Highlights significant changes in intensity.
*   **Edge Detection**: Identifies boundaries within the image.
*   **Sharpen**: Enhances the contrast of edges.
*   **Box Blur**: Simple averaging blur.
*   **Gaussian Blur**: Weighted averaging blur for noise reduction.
*   **Unsharp Mask**: Enhances image details.

## Prerequisites

- **Meson**: The primary build system.
- **Ninja**: Fast build system backend (usually installed with Meson).
- **MPI**: A message passing interface implementation (e.g., MPICH, OpenMPI).
- **GCC/Clang**: Compiler with OpenMP support.

## Project Structure

```
├── meson.build     # Build configuration
├── build           # Output directory (created by build)
├── data            # Output directory
│   ├── values      # CSV files (time_data.csv)
│   └── plots       # Generated plots
├── images          # Input and output directory for BMP images
│   ├── base        # Place input BMP files here
│   └── [kernel]    # Output directories (automatically created)
├── scripts         # Utility scripts
└── src             # Source code
```

## Setup

First, create the necessary data and image directories:

```bash
./scripts/setup_project.sh
```

Then, initialize the build directory:

```bash
meson setup build
```

This will configure the project and detect your dependencies (MPI, OpenMP).

## Building and Running

We use Meson `run_target`s to execute benchmarks. These targets automatically handle compilation if needed.

### Standard Benchmarks

```bash
# Run Serial benchmark
meson compile -C build run_serial

# Run Parallel Multithreaded benchmark
meson compile -C build run_multithreaded

# Run Parallel Distributed Filesystem benchmark
meson compile -C build run_distributed

# Run Parallel Shared Filesystem benchmark
meson compile -C build run_shared

# Run Parallel Task Pool benchmark
meson compile -C build run_task_pool

# Run All benchmarks sequentially
meson compile -C build run_all
```

### Manual Compilation

If you just want to build the executable without running:

```bash
meson compile -C build
```

The binary will be located at `build/mpi_omp_convolution`.

### Manual Execution & Flags

You can run the benchmark binary directly to control which modes are executed.

```bash
# General syntax
build/mpi_omp_convolution -threads <threads> [mode flags]
```

**Flags:**
*   `-threads <n>`      : Set number of OpenMP threads
*   `-serial`           : Run Serial benchmark (required for verification)
*   `-multithreaded`    : Run Parallel Multithreaded benchmark
*   `-distributed`      : Run Parallel Distributed Filesystem benchmark
*   `-shared`           : Run Parallel Shared Filesystem benchmark
*   `-task_pool`        : Run Parallel Task Pool benchmark
*   `-all`              : Run All benchmarks
*   `--help`            : Show usage

**Examples:**

```bash
# Run Serial benchmark (generates reference images)
build/mpi_omp_convolution -serial

# Run Distributed benchmark with 4 MPI processes and 4 OpenMP threads per rank
mpirun -n 4 build/mpi_omp_convolution -threads 4 -distributed

# Run All benchmarks
mpirun -n 4 build/mpi_omp_convolution -threads 4 -all
```

> **Note:** The parallel modes verify their output against the serial output. You must run the Serial benchmark (`-serial`) at least once to generate the reference images, otherwise verification will fail.

## Configuration

You can configure the optimization flags and the number of processes/threads directly in `meson.build`.

### Optimization Flags
The project is configured with aggressive optimization by default:
```meson
add_project_arguments('-O3', '-march=native', language : 'c')
```

### Process & Thread Counts
To change the number of MPI processes or OpenMP threads used by the run targets, edit the variables at the top of `meson.build`:

```meson
# Configuration Variables
mpi_processes = '8'
omp_threads = '8'
```

Configuring these in `meson.build` ensures that all `run_*` commands use your desired parallelism settings.

## Project Utilities

We provide several utility targets and scripts to help manage the project artifacts. You can run these via Meson or directly using the scripts in the `scripts/` directory.

### Cleaning & Process Management

| Task               | Meson Target                          | Script                      | Description                                               |
| ------------------ | ------------------------------------- | --------------------------- | --------------------------------------------------------- |
| **Clean Images**   | `meson compile -C build clean_images` | `./scripts/clear_images.sh` | Removes generated images in `images/` (preserves `base`). |
| **Clean Data**     | `meson compile -C build clean_data`   | `./scripts/clear_data.sh`   | Removes generated CSV files in `data/`.                   |
| **Kill Processes** | `meson compile -C build kill`         | `./scripts/kill_process.sh` | Kills running `mpi_omp_convolution` processes.            |

### Plotting & Analysis

To visualize the benchmark results, you can use the plotting script. This requires `pandas` and `matplotlib`.

**Generate Plots:**

```bash
# Using Meson
meson compile -C build plot

# Using Script
python3 scripts/plot.py
```

**Run Benchmark Sweep:**

To run a comprehensive sweep of benchmarks across different cluster sizes and thread counts:

```bash
# Using Meson
meson compile -C build run_sweep

# Using Script (requires paths to mpirun and executable)
./scripts/run_sweep.sh mpirun build/mpi_omp_convolution
```

## Cleaning

To clean the build artifacts:

```bash
meson compile --clean -C build
```

Or simply remove the build directory and re-setup if you want a fresh start:
```bash
rm -rf build
meson setup build
```
