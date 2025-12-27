#!/bin/bash
# Kills all instances of the project executable

TARGET="mpi_omp_convolution"

echo "Attempting to kill processes matching: $TARGET"

# pkill returns 0 if at least one process was matched and signaled
if pkill -f "$TARGET"; then
    echo "Processes killed successfully."
else
    echo "No matching processes found."
fi
