
#include <stdio.h>
#include "example_mpi_lib.h"


int this_is_another_sample;

void example_function_callback();

int main(int argc, char** argv){

    sample_mpi_init(NULL,NULL);
    sample_mpi_recv(0);
    sample_mpi_finalize();
}
