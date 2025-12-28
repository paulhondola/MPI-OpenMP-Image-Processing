#!/bin/sh

# Arguments:
# $1: Path to mpirun
# $2: Path to the executable

MPIRUN="$1"
EXE="$2"

if [ -z "$MPIRUN" ] || [ -z "$EXE" ]; then
    echo "Usage: $0 <mpirun_path> <exe_path>"
    exit 1
fi

CLUSTERS="1 2 4 8"
THREADS="2 4 8"

echo "Starting Benchmark Sweep..."

for c in $CLUSTERS; do
    for t in $THREADS; do
        echo "---------------------------------------------------"
        echo "Configuration: Clusters=$c, Threads=$t"
        echo "---------------------------------------------------"
        "$MPIRUN" -n "$c" "$EXE" -threads "$t" -all
    done
done

echo "Sweep Completed."
