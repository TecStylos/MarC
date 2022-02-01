#!/bin/bash

./build.sh RelWithDebInfo

rm callgrind.out.*

valgrind --tool=callgrind --dump-instr=yes --collect-jumps=yes ./bin/RelWithDebInfo/MarCmd/MarCmd --verbose --closeonexit examples/fibonacci.mca
