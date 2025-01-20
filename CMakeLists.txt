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

add_library(sample_mpi SHARED sample/example_mpi_lib.c)
target_link_libraries(sample_mpi PRIVATE ancor_programm_traits)

add_executable(example_app sample/example_app.c)
target_link_libraries(example_app PRIVATE sample_mpi)

add_library(normal_mpi SHARED sample/normal_mpi_lib.c)



