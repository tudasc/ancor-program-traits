cmake_minimum_required(VERSION 3.25)
project(ancor_programm_traits C)

set(CMAKE_C_STANDARD 11)

include(CTest)
enable_testing()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter")


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
endif ()

find_library(LIBDL_LOCATION libdl.so)





# the main target
add_library(ancor_programm_traits SHARED program_traits.c program_traits.h)
target_link_libraries(ancor_programm_traits ${GLIB_LDFLAGS})
target_link_libraries(ancor_programm_traits dl)


# so that generated and non generated files can include each other in either way:
target_include_directories(ancor_programm_traits PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(ancor_programm_traits PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# derived targets may want to include generated and non generated files:
set(PROGRAM_TRAITS_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}:${CMAKE_CURRENT_SOURCE_DIR}")

set(LIB_PROGRAM_TRAITS_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_SHARED_LIBRARY_PREFIX}ancor_programm_traits${CMAKE_SHARED_LIBRARY_SUFFIX})

option(HOOK_DLOPEN "Enable to hook dlopen, Off aborts analysis on detection of dlopen" On)
option(HOOK_MPROTECT "Enable to hook mprotect, Off aborts analysis on detection of mprotect" On)
if (HOOK_DLOPEN)
    include(ExternalProject)
    # remove quotation marks
    separate_arguments(C_FLAGS_EXPAND UNIX_COMMAND "${CMAKE_C_FLAGS}")
    ExternalProject_Add(
            plthook
            GIT_REPOSITORY https://github.com/kubo/plthook.git
            GIT_TAG 2858946
            CONFIGURE_COMMAND ""
            BUILD_IN_SOURCE 1
            BUILD_COMMAND ${CMAKE_C_COMPILER} ${C_FLAGS_EXPAND} -shared -fPIC -o libplthook.so plthook_elf.c -ldl
            INSTALL_COMMAND ""
            # need to declare the build byproducts for ninja
            BUILD_BYPRODUCTS <SOURCE_DIR>/libplthook.so
    )
    ExternalProject_Get_Property(plthook SOURCE_DIR)
    set(PLTHOOK_DIR ${SOURCE_DIR})
    set(LIB_PLTHOOK_LOCATION ${PLTHOOK_DIR}/libplthook.so)

    target_link_libraries(ancor_programm_traits ${PLTHOOK_DIR}/libplthook.so)
    target_include_directories(ancor_programm_traits PUBLIC ${PLTHOOK_DIR})
    add_dependencies(ancor_programm_traits plthook)
endif ()

# configure header based on options
configure_file(program_traits.h.in program_traits.h @ONLY)


add_subdirectory(tests)
add_subdirectory(sample)
