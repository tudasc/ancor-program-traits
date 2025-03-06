#!/bin/bash

# $1 = mpi header to patch
# $2 = patch file
# $3 = markers.h file

# Apply the patch file
patch -b $1 $2

# Insert markers.h content
sed -i "/REPLACE_WITH_MARKERS_H_CONTENT/{ r $3
d
}" $1