#!/bin/bash

read -p "Configuration: " CONFIG

if [ "${CONFIG}" = "" ]; then
    echo "No Configuration selected. Using 'Release'."
    BIN_DIR="bin/Release"
else
    BIN_DIR="bin/${CONFIG}"
fi

echo "Creating directory '${BIN_DIR}'..."
mkdir -p ${BIN_DIR}

echo "Changing directory to '${BIN_DIR}'..."
cd ${BIN_DIR}

if [ "${CONFIG}" = "" ]; then
    echo "Running CMake without a build type..."
    cmake ../..
else
    echo "Running CMake with build type '${CONFIG}'"
    cmake ../.. -DCMAKE_BUILD_TYPE=${CONFIG}
fi

echo "Running make..."
make
