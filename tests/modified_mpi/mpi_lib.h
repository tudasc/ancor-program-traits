#ifndef EXAMPLE_MPI_LIB_H
#define EXAMPLE_MPI_LIB_H

#include <assert.h>


void sample_mpi_init(int *argc, char ***argv);

void sample_mpi_recv(int tag);

void sample_mpi_finalize();


#ifndef USE_WILDCARD
#include "markers.h"
marker(no_wildcard)

#else
#define MATCHING_WILDCARD -1

#endif

#endif // end include guard

