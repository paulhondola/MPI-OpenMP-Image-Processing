# Visualization Tools

This directory contains Python scripts for analyzing and visualizing benchmark results from the MPI-OpenMP Image Processing project.

## Setup

This project is managed with `uv`. To install dependencies:

```bash
uv sync
```

## Usage

### Generate Plots

To generate performance plots from the speedup data:

```bash
uv run main.py
```

Arguments:
- `--data`: Path to the input CSV file (default: `../data/chrono/speedups_data.csv`)
- `--output`: Directory to save the plots (default: `plots`)

## Available Plots

The script generates the following types of plots in the output directory:

| specific configuration | Plot Type             | Description                                  |
| ---------------------- | --------------------- | -------------------------------------------- |
| `speedup_size_*.png`   | Speedup vs Image Size | Performance scaling with increasing workload |
| `scaling_threads.png`  | Speedup vs Threads    | Strong scaling within a single node (OpenMP) |
| `scaling_clusters.png` | Speedup vs Clusters   | Strong scaling across nodes (MPI)            |
| `efficiency.png`       | Parallel Efficiency   | Speedup per core vs Total Cores              |
