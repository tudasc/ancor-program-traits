
#include <stdio.h>
#include <assert.h>

#include "normal_mpi_lib.h"


int initialized=0;

void sample_mpi_init(int* argc, char*** argv){
    assert(initialized==0);
    initialized=1;
    printf("Normal MPI always is using wildcards");
}


void sample_mpi_recv(int tag){
    assert(initialized);
}

void sample_mpi_finalize(){
    assert(initialized);
    initialized=0;
}
