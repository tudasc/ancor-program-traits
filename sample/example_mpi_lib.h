#ifndef EXAMPLE_MPI_LIB_H
#define EXAMPLE_MPI_LIB_H

#include "../program_traits.h"
#include <assert.h>


void sample_mpi_init(int* argc, char*** argv);

void sample_mpi_recv(int tag);

void sample_mpi_finalize();


#ifndef USE_WILDCARD
#define MATCHING_WILDCARD -1 static_assert(0,"If you want to use matching wildcards: compile with -DUSE_WILDCARD");
void sample_mpi_init_additional_info();
marker(no_wildcard)
#define sample_mpi_init(argc, argv) call_if_present(sample_mpi_init_additional_info); sample_mpi_init(argc, argv)

#else
#define MATCHING_WILDCARD -1

#endif

#endif // end include guard

