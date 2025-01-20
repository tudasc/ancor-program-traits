
#include <stdio.h>
#include <assert.h>

#include "mpi_lib.h"


int initialized=0;

void sample_mpi_init(int* argc, char*** argv){
    assert(initialized==0);
    initialized=1;
    printf("Normal MPI always is using wildcards\n");
}


void sample_mpi_recv(int tag){
    assert(initialized);
    if (tag == MATCHING_WILDCARD) {
        printf("Allowed Wildcard usage\n");
    }
}

void sample_mpi_finalize(){
    assert(initialized);
    initialized=0;
}
