#!/bin/sh

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Path to the data directory (relative to scripts/)
DATA_DIR="$SCRIPT_DIR/../data/"

# Safety check: Ensure the data directory exists
if [ ! -d "$DATA_DIR" ]; then
    echo "Error: Data directory not found at '$DATA_DIR'"
    exit 1
fi

echo "Cleaning all data in '$DATA_DIR'..." 

rm -rf "$DATA_DIR"/*

echo "Cleanup complete."
