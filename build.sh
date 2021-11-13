#!/bin/bash

read -p "Configuration: " CONFIG

BIN_DIR="bin/${CONFIG}"

echo "Creating directory '${BIN_DIR}'..."
mkdir -p ${BIN_DIR}

echo "Running CMake in '${BIN_DIR}'..."
cd ${BIN_DIR}
cmake ../.. -DCMAKE_BUILD_TYPE=${CONFIG}

echo "Running make..."
make
