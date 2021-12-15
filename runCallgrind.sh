#!/bin/bash

valgrind --tool=callgrind --dump-instr=yes --collect-jumps=yes ./bin/RelWithDebInfo/MarCmd/MarCmd --verbose --closeonexit examples/fibonacci.mca
