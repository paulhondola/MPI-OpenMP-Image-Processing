#!/bin/sh

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Assuming the script is in /scripts, the images are in /images (one level up)
IMAGES_DIR="$SCRIPT_DIR/../images"

# Safety check: Ensure the images directory exists
if [ ! -d "$IMAGES_DIR" ]; then
    echo "Error: Images directory not found at '$IMAGES_DIR'"
    exit 1
fi

echo "Cleaning images in '$IMAGES_DIR' (preserving 'base')..."

# Find and delete all files/directories in images/ except 'base'
# -mindepth 1: Don't check the images dir itself
# -maxdepth 1: Only check top-level children
# -not -name base: Skip the 'base' folder
find "$IMAGES_DIR" -mindepth 1 -maxdepth 1 -not -name base -exec rm -rf {} +

echo "Cleanup complete."
