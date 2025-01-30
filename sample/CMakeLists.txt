
include(ExternalProject)

# wildcards inside of the MPI implementation are allowed, otherwise MPI will not compile
set(MPICH_CFLAGS "${CMAKE_C_FLAGS} -DUSE_WILDCARD")
set(MPICH_CXXFLAGS "${CMAKE_CXX_FLAGS} -DUSE_WILDCARD")

#set(MPICH_CONFIGURE_FLAGS "MPICHLIB_CFLAGS=${MPICH_CFLAGS} MPICHLIB_CXXFLAGS=${MPICH_CXXFLAGS}")
set(MPICH_CONFIGURE_FLAGS "")

ExternalProject_Add(
        mpich
        URL https://www.mpich.org/static/downloads/4.2.3/mpich-4.2.3.tar.gz
        URL_HASH MD5=b0c2a9690ce5325d69b7022219d94f64
        CONFIGURE_COMMAND
        <SOURCE_DIR>/configure --prefix <INSTALL_DIR> CC=${CMAKE_C_COMPILER} CFLAGS=${MPICH_CFLAGS} CXXFLAGS=${MPICH_CXXFLAGS} ${MPICH_CONFIGURE_FLAGS}
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

#add_library(MPICH_LIBRARY SHARED IMPORTED GLOBAL)
#set_property(TARGET MPICH_LIBRARY PROPERTY IMPORTED_LOCATION ${MPICH_INSTALL_DIR}/lib/libmpi.so)
set(MPICH_LIBRARY ${MPICH_INSTALL_DIR}/lib/libmpi.so)

#add_executable(MPICC_TARGET IMPORTED GLOBAL)
#set_property(TARGET MPICC_TARGET PROPERTY IMPORTED_LOCATION ${MPICH_INSTALL_DIR}/bin/mpicc)
set(MPICC ${MPICH_INSTALL_DIR}/bin/mpicc)

#add_executable(MPICXX IMPORTED GLOBAL)
#set_property(TARGET MPICXX PROPERTY IMPORTED_LOCATION ${MPICH_INSTALL_DIR}/bin/mpicxx)
set(MPICXX ${MPICH_INSTALL_DIR}/bin/mpicxx)

#add_executable(MPIRUN IMPORTED GLOBAL)
#set_property(TARGET MPIRUN PROPERTY IMPORTED_LOCATION ${MPICH_INSTALL_DIR}/bin/mpirun)
set(MPIRUN ${MPICH_INSTALL_DIR}/bin/mpirun)


add_library(mpi_wrapper_lib SHARED mpi_wrapper.c mpi_wrapper.h)
# configure the libraries ignored in wrapper lib
find_library(LIBVERBS_LOCATION libibverbs.so)
find_library(LIBTLDL_LOCATION libltdl.so)
set(LIBMPI_LOCATION ${MPICH_LIBRARY})
configure_file(mpi_wrapper.h.in mpi_wrapper.h @ONLY)

target_include_directories(mpi_wrapper_lib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

target_include_directories(mpi_wrapper_lib PUBLIC ${MPICH_INSTALL_DIR}/include)
target_link_libraries(mpi_wrapper_lib ${MPICH_LIBRARY})
add_dependencies(mpi_wrapper_lib mpich)

target_link_libraries(mpi_wrapper_lib ancor_programm_traits)
get_target_property(LIB_SRC_DIR ancor_programm_traits SOURCE_DIR)
target_include_directories(mpi_wrapper_lib PRIVATE ${LIB_SRC_DIR})


add_test(NAME mpi_hello_world_with_wrapper
        # compile and run with our wrapper loaded as a normal pmpi tool
        COMMAND bash -c "${MPICC} ${CMAKE_CURRENT_SOURCE_DIR}/mpi_hello_world.c ; \
            LD_PRELOAD=$<TARGET_FILE:mpi_wrapper_lib> ${MPIRUN} -n 2 ./a.out ; "
        PROCESSORS 2
        PASS_REGULAR_EXPRESSION "MPI Initialized with wildcards DISabled"
)
#without LD_PRELOAD
add_test(NAME mpi_hello_world
        # compile and run with our wrapper loaded as a normal pmpi tool
        COMMAND bash -c "${MPIRUN} -n 2 ./a.out"
        PROCESSORS 2
        PASS_REGULAR_EXPRESSION "Hello World from Rank 0"
)
# the other test will build the binary, so it needs to successfully complete first
set_tests_properties(mpi_hello_world PROPERTIES DEPENDS mpi_hello_world_with_wrapper)

option(BUILD_MINI_APPS "Build Mini-Apps with the PMPI wrapper" ON)


if (BUILD_MINI_APPS)

    ExternalProject_Add(
            lulesh
            GIT_REPOSITORY https://github.com/LLNL/LULESH.git
            GIT_TAG 3e01c40b3281aadb7f996525cdd4a3354f6d3801
            CMAKE_ARGS
            -DCMAKE_C_COMPILER=${MPICC}
            -DCMAKE_CXX_COMPILER=${MPICXX}
            -DMPI_ROOT=${MPICH_INSTALL_DIR}
            DEPENDS mpich
            INSTALL_COMMAND ""
    )
    ExternalProject_Get_Property(lulesh BINARY_DIR)
    set(LULESH_EXE ${BINARY_DIR}/lulesh2.0)

    #without LD_PRELOAD
    add_test(NAME lulesh
            COMMAND bash -c "${MPIRUN} -n 8 ${LULESH_EXE} -s 10 -i 10"

    )
    set_tests_properties(lulesh PROPERTIES
            ENVIRONMENT "OMP_NUM_THREADS=1"
            PROCESSORS 8
            PASS_REGULAR_EXPRESSION "Run completed:")

    #with out wrapper
    add_test(NAME lulesh_with_wrapper
            # run with our wrapper loaded as a normal pmpi tool
            COMMAND bash -c "LD_PRELOAD=$<TARGET_FILE:mpi_wrapper_lib> ${MPIRUN} -n 8 ${LULESH_EXE} -s 10 -i 10"

    )
    set_tests_properties(lulesh_with_wrapper PROPERTIES
            ENVIRONMENT "OMP_NUM_THREADS=1"
            PROCESSORS 8
            PASS_REGULAR_EXPRESSION "MPI Initialized with wildcards DISabled")

    # AMG
    ExternalProject_Add(
            AMG
            GIT_REPOSITORY https://github.com/LLNL/AMG.git
            GIT_TAG 1.2
            CONFIGURE_COMMAND sed -i s!^CC[[:space:]]*=.*!CC=${MPICC}! <SOURCE_DIR>/Makefile.include
            BUILD_IN_SOURCE 1
            BUILD_COMMAND make -j ${CMAKE_BUILD_PARALLEL_LEVEL}
            DEPENDS mpich
            INSTALL_COMMAND ""
    )
    ExternalProject_Get_Property(AMG SOURCE_DIR)
    set(AMG_EXE ${SOURCE_DIR}/test/amg)

    add_test(NAME amg
            COMMAND bash -c "${MPIRUN} -n 2 ${AMG_EXE}")
    set_tests_properties(amg PROPERTIES
            PROCESSORS 2
            ENVIRONMENT "OMP_NUM_THREADS=1")

    #with out wrapper
    add_test(NAME amg_with_wrapper
            # run with our wrapper loaded as a normal pmpi tool
            COMMAND bash -c "LD_PRELOAD=$<TARGET_FILE:mpi_wrapper_lib> ${MPIRUN} -n 2 ${AMG_EXE}")
    set_tests_properties(amg_with_wrapper PROPERTIES
            PROCESSORS 2
            ENVIRONMENT "OMP_NUM_THREADS=1"
            PASS_REGULAR_EXPRESSION "MPI Initialized with wildcards DISabled")


endif ()

