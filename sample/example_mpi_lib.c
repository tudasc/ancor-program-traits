
#include <stdio.h>
#include <assert.h>

#include "../program_traits.h"
#define USE_WILDCARD
marker(no_wildcard)
// the usages of wildcard inside the MPI lib itself is permitted

#include "example_mpi_lib.h"
#include "../program_traits.h"

int allow_wildcard_usage=1;
int initialized=0;

void sample_mpi_init(int* argc, char*** argv){
    assert(initialized==0);
    initialized=1;
    printf("MPI Initialized with wildcards %s\n",allow_wildcard_usage?"ENabled":"DISabled");
}

void sample_mpi_init_additional_info(){
    //TODO implement
}

void sample_mpi_recv(int tag){
    assert(initialized);
    assert(tag!=MATCHING_WILDCARD || !allow_wildcard_usage);
}

void sample_mpi_finalize(){
    assert(initialized);
    initialized=0;
}
