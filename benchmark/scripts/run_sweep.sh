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

# list of "clusters:threads" configurations
CONFIGS="2:1 2:2 2:4 4:1 4:2 8:1"

echo "Starting Benchmark Sweep..."

for config in $CONFIGS; do
    # Split the config string "c:t"
    c=${config%:*}
    t=${config#*:}
    
    echo "---------------------------------------------------"
    echo "Configuration: Clusters=$c, Threads=$t"
    echo "---------------------------------------------------"
    "$MPIRUN" -n "$c" "$EXE" --threads "$t" --all
done

echo "Sweep Completed."
