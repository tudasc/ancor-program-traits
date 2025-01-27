
find_package(MPI REQUIRED)


add_library(mpi_wrapper_lib SHARED mpi_wrapper_lib.c)

target_link_libraries(mpi_wrapper_lib MPI::MPI_C)

target_link_libraries(mpi_wrapper_lib ancor_programm_traits)

