#ifndef NORMAL_MPI_LIB_H
#define NORMAL_MPI_LIB_H

void sample_mpi_init(int* argc, char*** argv);

void sample_mpi_recv(int tag);

void sample_mpi_finalize();

#define MATCHING_WILDCARD -1

#endif // end include guard

