#!/bin/bash

INSTALL_DIR="$1"
SOURCE_DIR="$2"
PARALLEL_LEVEL="$3"
C_COMPILER="$4"

# all other args are configure args
shift 4

if [ ! -f "$INSTALL_DIR/bin/mpicc" ]; then
    $SOURCE_DIR/configure --prefix=$INSTALL_DIR CC=$C_COMPILER "$@" && make -j $PARALLEL_LEVEL
    exit $?
fi