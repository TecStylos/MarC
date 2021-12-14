#!/bin/bash

valgrind --tool=callgrind ./bin/RelWithDebInfo/MarCmd/MarCmd --verbose --closeonexit examples/fibonacci.mca
