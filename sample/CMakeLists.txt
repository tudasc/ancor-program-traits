
include(ExternalProject)

set(MPICH_CONFIGURE_FLAGS "")

# wildcards inside of the MPI implementation are allowed, otherwise MPI will not compile
set(MPICH_CFLAGS "${CMAKE_C_FLAGS} -DUSE_WILDCARD")

ExternalProject_Add(
        mpich
        URL https://www.mpich.org/static/downloads/4.2.3/mpich-4.2.3.tar.gz
        URL_HASH MD5=b0c2a9690ce5325d69b7022219d94f64
        CONFIGURE_COMMAND
        <SOURCE_DIR>/configure --prefix <INSTALL_DIR> CC=${CMAKE_C_COMPILER} FLAGS=${MPICH_CFLAGS} ${MPICH_CONFIGURE_FLAGS}
        PATCH_COMMAND patch -b <SOURCE_DIR>/src/include/mpi.h.in ${CMAKE_CURRENT_SOURCE_DIR}/mpi.h.in.patch
        BUILD_COMMAND make -j ${CMAKE_BUILD_PARALLEL_LEVEL}
        INSTALL_COMMAND make install
        DOWNLOAD_EXTRACT_TIMESTAMP true
        # need to declare the build byproducts for ninja
        BUILD_BYPRODUCTS <INSTALL_DIR>/lib/libmpi.so <INSTALL_DIR>/bin/mpicc <INSTALL_DIR>/bin/mpicxx <INSTALL_DIR>/bin/mpirun
)

#make sure the build and downloaded MPI s used
# therefore: do not use #find_package(MPI REQUIRED)
ExternalProject_Get_Property(mpich INSTALL_DIR)
set(MPICH_INSTALL_DIR ${INSTALL_DIR})

add_library(MPICH_LIBRARY SHARED IMPORTED GLOBAL)
set_property(TARGET MPICH_LIBRARY PROPERTY IMPORTED_LOCATION ${MPICH_INSTALL_DIR}/lib/libmpi.so)

add_executable(MPICC IMPORTED GLOBAL)
set_property(TARGET MPICC PROPERTY IMPORTED_LOCATION ${MPICH_INSTALL_DIR}/bin/mpicc)

add_executable(MPICXX IMPORTED GLOBAL)
set_property(TARGET MPICXX PROPERTY IMPORTED_LOCATION ${MPICH_INSTALL_DIR}/bin/mpicxx)

add_executable(MPIRUN IMPORTED GLOBAL)
set_property(TARGET MPIRUN PROPERTY IMPORTED_LOCATION ${MPICH_INSTALL_DIR}/bin/mpirun)


# configure the libraries ignored in wrapper lib
find_library(LIBVERBS_LOCATION libibverbs.so)
find_library(LIBTLDL_LOCATION libltdl.so)
set(LIBMPI_LOCATION MPICH_LIBRARY)
configure_file(mpi_wrapper.h.in mpi_wrapper.h @ONLY)

add_library(mpi_wrapper_lib SHARED mpi_wrapper.c mpi_wrapper.h)

target_include_directories(mpi_wrapper_lib PUBLIC ${MPICH_INSTALL_DIR}/include)
target_link_libraries(mpi_wrapper_lib MPICH_LIBRARY)
add_dependencies(mpi_wrapper_lib mpich)

target_link_libraries(mpi_wrapper_lib ancor_programm_traits)
get_target_property(LIB_SRC_DIR ancor_programm_traits SOURCE_DIR)
target_include_directories(mpi_wrapper_lib PRIVATE ${LIB_SRC_DIR})


#
#add_executable(mpi_hello_wolrd mpi_hello_world.c)
#TODO set compiler to mpicc!


add_test(mpi_hello_world COMMAND MPICC ${CMAKE_CURRENT_SOURCE_DIR}/mpi_hello_world.c && LD_PRELOAD=mpi_wrapper_lib MPIRUN ./a.out)
