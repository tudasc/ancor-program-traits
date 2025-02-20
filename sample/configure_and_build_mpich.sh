#!/bin/bash

INSTALL_DIR="$1"
SOURCE_DIR="$2"
C_COMPILER="$3"
CONFIGURE_FLAGS="$4"
PARALLEL_LEVEL="$5"

if [ ! -f "$INSTALL_DIR/bin/mpicc" ]; then
    $SOURCE_DIR/configure --prefix="$INSTALL_DIR" CC="$C_COMPILER" $CONFIGURE_FLAGS
    make -j "$PARALLEL_LEVEL"
fi