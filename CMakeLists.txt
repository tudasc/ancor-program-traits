cmake_minimum_required(VERSION 3.30)
project(ancor_programm_traits C)

set(CMAKE_C_STANDARD 11)


set(CMAKE_C_FLAGS "-Wall -Wextra -Wno-unused-parameter")

find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)

include_directories(${GLIB_INCLUDE_DIRS})


# the main target
add_library(ancor_programm_traits SHARED program_traits.c)
target_link_libraries(ancor_programm_traits ${GLIB_LDFLAGS})


# the example to showcase/test how it works
# TODO later: add these configurations as ctests

add_library(sample_mpi SHARED sample/example_mpi_lib.c)
target_link_libraries(sample_mpi PRIVATE ancor_programm_traits)

add_library(normal_mpi SHARED sample/normal_mpi_lib.c)



# expected to work with wildcards disabled
add_executable(example_app sample/example_app.c)
target_link_libraries(example_app PRIVATE sample_mpi)


# expected to work with wildcards enabled
add_executable(example_app_wildcard sample/example_app.c)
target_link_libraries(example_app_wildcard PRIVATE sample_mpi)
target_compile_options(example_app_wildcard PUBLIC -DUSE_WILDCARD -DTAG_TO_USE=MATCHING_WILDCARD)

#expected to FAIL compilation
add_executable(example_app_wildcard_fail sample/example_app.c)
target_link_libraries(example_app_wildcard_fail PRIVATE sample_mpi)
target_compile_options(example_app_wildcard_fail PUBLIC -DTAG_TO_USE=MATCHING_WILDCARD)

