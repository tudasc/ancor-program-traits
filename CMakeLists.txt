cmake_minimum_required(VERSION 3.25)
project(ancor_programm_traits C)

set(CMAKE_C_STANDARD 11)

include(CTest)
enable_testing()

set(CMAKE_C_FLAGS "-Wall -Wextra -Wno-unused-parameter")

find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)

include_directories(${GLIB_INCLUDE_DIRS})


# the main target
add_library(ancor_programm_traits SHARED program_traits.c)
target_link_libraries(ancor_programm_traits ${GLIB_LDFLAGS})
target_link_libraries(ancor_programm_traits dl)


add_subdirectory(tests)
add_subdirectory(sample)
