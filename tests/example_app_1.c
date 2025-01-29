
#include <stdio.h>

#include "mpi_lib.h"

#ifndef TAG_TO_USE
#define TAG_TO_USE 42
#endif

int main(int argc, char** argv){

    sample_mpi_init(NULL,NULL);
    sample_mpi_recv( TAG_TO_USE );
    sample_mpi_finalize();
}
