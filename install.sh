#!/bin/bash

# Call makefile first
make

# Variables
LIB_NAME="wt"
INCLUDE_DIR="/usr/include/${LIB_NAME}"
LIB_DIR="/usr/local/lib"

# Create directories
sudo mkdir -p $INCLUDE_DIR

# Copy header files to include directory
sudo cp src/*.h $INCLUDE_DIR

# Copy library binaries to library directory
sudo cp out/lib${LIB_NAME}.a $LIB_DIR

# Update cache if installing shared libraries
# sudo ldconfig

echo "Library and headers installed successfully."