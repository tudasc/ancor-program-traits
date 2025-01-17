cmake_minimum_required(VERSION 3.30)
project(ancor_programm_traits C)

set(CMAKE_C_STANDARD 11)


set(CMAKE_C_FLAGS "-Wall -Wextra -Wno-unused-parameter")

find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)

include_directories(${GLIB_INCLUDE_DIRS})


# the main target
add_library(ancor_programm_traits SHARED library.c)
target_link_libraries(ancor_programm_traits ${GLIB_LDFLAGS})

# the example to showcase/test how it works
add_executable(example sample/example.c)
target_link_libraries(example PRIVATE ancor_programm_traits)
