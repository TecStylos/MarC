#!/bin/bash

if [ $# -eq 0 ]; then
    read -p "Configuration: " CONFIG
else
    CONFIG="$1"
fi

if [ "${CONFIG}" = "" ]; then
    echo "No Configuration selected. Using 'Release'."
    BIN_DIR="bin/Release"
else
    if [ "${CONFIG}" != "Debug" ]; then
        if [ "${CONFIG}" != "Release" ]; then
            if [ "${CONFIG}" != "RelWithDebInfo" ]; then
                echo "Invalid Configuration selected!"
                exit
            fi
        fi
    fi
    BIN_DIR="bin/${CONFIG}"
fi

echo "Creating directory '${BIN_DIR}'..."
mkdir -p ${BIN_DIR}

echo "Changing directory to '${BIN_DIR}'..."
cd ${BIN_DIR}

if [ "${CONFIG}" = "" ]; then
    echo "Running CMake without a build type..."
    cmake ../.. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
else
    echo "Running CMake with build type '${CONFIG}'"
    cmake ../.. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=${CONFIG}
fi

echo "Running make..."
make
