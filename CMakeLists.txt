cmake_minimum_required(VERSION 3.25)
project(ancor_programm_traits C)

set(CMAKE_C_STANDARD 11)

include(CTest)
enable_testing()

set(CMAKE_C_FLAGS "-Wall -Wextra -Wno-unused-parameter")

find_package(PkgConfig REQUIRED)
pkg_search_module(GLIB REQUIRED glib-2.0)

include_directories(${GLIB_INCLUDE_DIRS})


# find special libraries to ignore when searching for dlopen and mprotect
#find_library(LIBC_LOCATION libc.so) # does not work, as it points to the gnu linker script and not the object file
# but ldd will show the correct path
execute_process(
        COMMAND ldd ${CMAKE_C_COMPILER}
        COMMAND grep "libc.so"
        COMMAND awk "{print \$3}"
        OUTPUT_VARIABLE LIBC_LOCATION
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

if (NOT LIBC_LOCATION)
    message(FATAL_ERROR "Could not determine libc shared library path")
endif()

find_library(LIBDL_LOCATION libdl.so)

configure_file(program_traits.h.in program_traits.h @ONLY)

# the main target
add_library(ancor_programm_traits SHARED program_traits.c program_traits.h)
target_link_libraries(ancor_programm_traits ${GLIB_LDFLAGS})
target_link_libraries(ancor_programm_traits dl)

# so that generated and non generated files can include each other in either way:
target_include_directories(ancor_programm_traits PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(ancor_programm_traits PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# derived targets may want to include generated and non generated files:
set(PROGRAM_TRAITS_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}:${CMAKE_CURRENT_SOURCE_DIR}")


add_subdirectory(tests)
add_subdirectory(sample)
