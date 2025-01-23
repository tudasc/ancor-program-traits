
#include <stdio.h>
#include <assert.h>

#include "wrapper_lib.h"
#include "mpi_lib.h"

#ifndef TAG_TO_USE
#define TAG_TO_USE 42
#endif

int initialized = 0;

void init_library() {
    sample_mpi_init(NULL, NULL);
}

void perform_operation() {
    sample_mpi_recv(TAG_TO_USE);
}

void finish_library() {
    sample_mpi_finalize();
}
