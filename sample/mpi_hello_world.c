/*The Parallel Hello World Program*/
#include <stdio.h>
#include <mpi.h>

// compile /home/tim/mpich/install/bin/mpicc mpi_hello_world.c
// run LD_PRELOAD=/home/tim/ancor-programm-traits/cmake-build-debug/sample/pmpi_wrapper_test/libmpi_wrapper_lib.so /home/tim/mpich/install/bin/mpirun -n 2 ./a.out
// without LD_PRELOAD: our mpi wrapper checking for wildcard usage is not used

int main(int argc, char **argv)
{
    int rank;

    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank==0){
      printf("Hello World from Rank %d\n",rank);
    }

    MPI_Finalize();
}