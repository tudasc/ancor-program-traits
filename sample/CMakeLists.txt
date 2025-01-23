# the example to showcase/test how it works

# modified stub library for testing
add_library(modified_mpi SHARED modified_mpi/mpi_lib.c)
target_link_libraries(modified_mpi PRIVATE ancor_programm_traits)

# normal stub library
add_library(normal_mpi SHARED normal_mpi/mpi_lib.c)

# test case naming sceme:
# default or modified header
# default or modified Library
# uses wildcards
# D_D_N = default default, no wildcards
# M_M_W modified,modified, has wildcards

# expected to work with wildcards disabled
add_executable(M_M_N example_app_1.c)
target_include_directories(M_M_N PUBLIC modified_mpi)
target_link_libraries(M_M_N PRIVATE modified_mpi)
add_test(NAME M_M_N COMMAND M_M_N)
set_tests_properties(M_M_N PROPERTIES
        PASS_REGULAR_EXPRESSION "MPI Initialized with wildcards DISabled"
)

# expected to work with wildcards enabled
add_executable(M_M_W example_app_1.c)
target_include_directories(M_M_W PUBLIC modified_mpi)
target_link_libraries(M_M_W PRIVATE modified_mpi)
target_compile_definitions(M_M_W PUBLIC USE_WILDCARD)
target_compile_definitions(M_M_W PUBLIC TAG_TO_USE=MATCHING_WILDCARD)
add_test(NAME M_M_W COMMAND M_M_W)
set_tests_properties(M_M_W PROPERTIES
        PASS_REGULAR_EXPRESSION "MPI Initialized with wildcards ENabled"
)


#expected to FAIL compilation
add_executable(M_M_W_fail example_app_1.c)
target_include_directories(M_M_W_fail PUBLIC modified_mpi)
target_link_libraries(M_M_W_fail PRIVATE modified_mpi)
# omitting this should result in build failure
#target_compile_definitions(M_M_W_fail PUBLIC USE_WILDCARD)
target_compile_definitions(M_M_W_fail PUBLIC TAG_TO_USE=MATCHING_WILDCARD)
# dont try to build it on default cmake invocation as it will fail
set_target_properties(M_M_W_fail PROPERTIES
        EXCLUDE_FROM_ALL TRUE
        EXCLUDE_FROM_DEFAULT_BUILD TRUE)
add_test(NAME M_M_W_fail COMMAND ${CMAKE_COMMAND} --build . --target M_M_W_fail --config $<CONFIGURATION>
WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(M_M_W_fail PROPERTIES WILL_FAIL TRUE)


add_executable(N_M_N example_app_1.c)
target_include_directories(N_M_N PUBLIC normal_mpi)
target_link_libraries(N_M_N PRIVATE modified_mpi)
add_test(NAME N_M_N COMMAND N_M_N)
# if compiled with normal header: marker is not present, need to assume wildcard usage
set_tests_properties(N_M_N PROPERTIES
        PASS_REGULAR_EXPRESSION "MPI Initialized with wildcards ENabled"
)

add_executable(N_M_W example_app_1.c)
target_include_directories(N_M_W PUBLIC normal_mpi)
target_link_libraries(N_M_W PRIVATE modified_mpi)
# has no effect in normal header
#target_compile_definitions(N_M_W PUBLIC USE_WILDCARD)
target_compile_definitions(N_M_W PUBLIC TAG_TO_USE=MATCHING_WILDCARD)
add_test(NAME N_M_W COMMAND N_M_W)
set_tests_properties(N_M_W PROPERTIES
        PASS_REGULAR_EXPRESSION "MPI Initialized with wildcards ENabled"
)



add_executable(N_N_N example_app_1.c)
target_include_directories(N_N_N PUBLIC normal_mpi)
target_link_libraries(N_N_N PRIVATE normal_mpi)
add_test(NAME N_N_N COMMAND N_N_N)
set_tests_properties(N_N_N PROPERTIES
        PASS_REGULAR_EXPRESSION "Normal MPI always is using wildcards"
)

add_executable(N_N_W example_app_1.c)
target_include_directories(N_N_W PUBLIC normal_mpi)
target_link_libraries(N_N_W PRIVATE normal_mpi)
# has no effect in normal header
#target_compile_definitions(N_N_W PUBLIC USE_WILDCARD)
target_compile_definitions(N_N_W PUBLIC TAG_TO_USE=MATCHING_WILDCARD)
add_test(NAME N_N_W COMMAND N_N_W)
set_tests_properties(N_N_W PROPERTIES
        PASS_REGULAR_EXPRESSION "Normal MPI always is using wildcards"
)


add_executable(M_N_N example_app_1.c)
target_include_directories(M_N_N PUBLIC modified_mpi)
target_link_libraries(M_N_N PRIVATE normal_mpi)
add_test(NAME M_N_N COMMAND M_N_N)
set_tests_properties(M_N_N PROPERTIES
        PASS_REGULAR_EXPRESSION "Normal MPI always is using wildcards"
)

add_executable(M_N_W example_app_1.c)
target_include_directories(M_N_W PUBLIC modified_mpi)
target_link_libraries(M_N_W PRIVATE normal_mpi)
target_compile_definitions(M_N_W PUBLIC USE_WILDCARD)
target_compile_definitions(M_N_W PUBLIC TAG_TO_USE=MATCHING_WILDCARD)
add_test(NAME M_N_W COMMAND M_N_W)
set_tests_properties(M_N_W PROPERTIES
        PASS_REGULAR_EXPRESSION "Normal MPI always is using wildcards"
)

# fail compilation
add_executable(M_N_W_fail example_app_1.c)
target_include_directories(M_N_W_fail PUBLIC modified_mpi)
target_link_libraries(M_N_W_fail PRIVATE normal_mpi)
# omit this will fail compilation
#target_compile_definitions(M_N_W_fail PUBLIC USE_WILDCARD)
target_compile_definitions(M_N_W_fail PUBLIC TAG_TO_USE=MATCHING_WILDCARD)
set_target_properties(M_N_W_fail PROPERTIES
        EXCLUDE_FROM_ALL TRUE
        EXCLUDE_FROM_DEFAULT_BUILD TRUE)
add_test(NAME M_N_W_fail COMMAND ${CMAKE_COMMAND} --build . --target M_N_W_fail --config $<CONFIGURATION>
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(M_N_W_fail PROPERTIES WILL_FAIL TRUE)

# test whats happening is MPI is hidden in a library

add_library(wrapper_normal_no SHARED wrapper_lib/wrapper_lib.c)
target_include_directories(wrapper_normal_no PUBLIC normal_mpi)
target_link_libraries(wrapper_normal_no PRIVATE normal_mpi)

add_library(wrapper_modified_no SHARED wrapper_lib/wrapper_lib.c)
target_include_directories(wrapper_modified_no PUBLIC modified_mpi)
target_link_libraries(wrapper_modified_no PRIVATE modified_mpi)

add_library(wrapper_normal_with SHARED wrapper_lib/wrapper_lib.c)
target_include_directories(wrapper_normal_with PUBLIC normal_mpi)
target_link_libraries(wrapper_normal_with PRIVATE normal_mpi)
# not needed with normal header
#target_compile_definitions(wrapper_normal_with PUBLIC USE_WILDCARD)
target_compile_definitions(wrapper_normal_with PUBLIC TAG_TO_USE=MATCHING_WILDCARD)

add_library(wrapper_modified_with SHARED wrapper_lib/wrapper_lib.c)
target_include_directories(wrapper_modified_with PUBLIC modified_mpi)
target_link_libraries(wrapper_modified_with PRIVATE modified_mpi)
target_compile_definitions(wrapper_modified_with PUBLIC USE_WILDCARD)
target_compile_definitions(wrapper_modified_with PUBLIC TAG_TO_USE=MATCHING_WILDCARD)


add_library(wrapper_modified_with_fail SHARED wrapper_lib/wrapper_lib.c)
target_include_directories(wrapper_modified_with_fail PUBLIC modified_mpi)
target_link_libraries(wrapper_modified_with_fail PRIVATE modified_mpi)
# make compilation fail
#target_compile_definitions(wrapper_modified_with_fail PUBLIC USE_WILDCARD)
target_compile_definitions(wrapper_modified_with_fail PUBLIC TAG_TO_USE=MATCHING_WILDCARD)
set_target_properties(wrapper_modified_with_fail PROPERTIES
        EXCLUDE_FROM_ALL TRUE
        EXCLUDE_FROM_DEFAULT_BUILD TRUE)
add_test(NAME wrapper_modified_with_fail COMMAND ${CMAKE_COMMAND} --build . --target wrapper_modified_with_fail --config $<CONFIGURATION>
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
set_tests_properties(wrapper_modified_with_fail PROPERTIES WILL_FAIL TRUE)

# test the SAME source code linked against the different version of the wrapper library
# note that the header of the Lib is always the same
add_executable(wrapper_test_1 example_app_2.c)
target_include_directories(wrapper_test_1 PUBLIC wrapper_lib)
target_link_libraries(wrapper_test_1 PRIVATE wrapper_normal_no)
add_test(NAME wrapper_test_1 COMMAND wrapper_test_1)
set_tests_properties(wrapper_test_1 PROPERTIES
        PASS_REGULAR_EXPRESSION "Normal MPI always is using wildcards"
)

add_executable(wrapper_test_2 example_app_2.c)
target_include_directories(wrapper_test_2 PUBLIC wrapper_lib)
target_link_libraries(wrapper_test_2 PRIVATE wrapper_normal_with)
add_test(NAME wrapper_test_2 COMMAND wrapper_test_2)
set_tests_properties(wrapper_test_2 PROPERTIES
        PASS_REGULAR_EXPRESSION "Normal MPI always is using wildcards"
)

add_executable(wrapper_test_3 example_app_2.c)
target_include_directories(wrapper_test_3 PUBLIC wrapper_lib)
target_link_libraries(wrapper_test_3 PRIVATE wrapper_modified_no)
add_test(NAME wrapper_test_3 COMMAND wrapper_test_3)
set_tests_properties(wrapper_test_3 PROPERTIES
        PASS_REGULAR_EXPRESSION "MPI Initialized with wildcards DISabled"
)

add_executable(wrapper_test_4 example_app_2.c)
target_include_directories(wrapper_test_4 PUBLIC wrapper_lib)
target_link_libraries(wrapper_test_4 PRIVATE wrapper_modified_with)
add_test(NAME wrapper_test_4 COMMAND wrapper_test_4)
set_tests_properties(wrapper_test_4 PROPERTIES
        PASS_REGULAR_EXPRESSION "MPI Initialized with wildcards ENabled"
)


add_executable(wrapper_test_5 example_app_3.c)
# the lib that contains dlopen
target_link_libraries(wrapper_test_5 PRIVATE dl)
add_dependencies(wrapper_test_5 wrapper_normal_no)
target_compile_definitions(wrapper_test_5 PUBLIC LIBRARY_TO_LOAD="$<TARGET_FILE:wrapper_normal_no>")
add_test(NAME wrapper_test_5 COMMAND wrapper_test_5)
set_tests_properties(wrapper_test_5 PROPERTIES
        PASS_REGULAR_EXPRESSION "Normal MPI always is using wildcards"
)

add_executable(wrapper_test_6 example_app_3.c)
# the lib that contains dlopen
target_link_libraries(wrapper_test_6 PRIVATE dl)
add_dependencies(wrapper_test_6 wrapper_normal_no)
target_compile_definitions(wrapper_test_6 PUBLIC LIBRARY_TO_LOAD="$<TARGET_FILE:wrapper_normal_with>")
add_test(NAME wrapper_test_6 COMMAND wrapper_test_6)
set_tests_properties(wrapper_test_6 PROPERTIES
        PASS_REGULAR_EXPRESSION "Normal MPI always is using wildcards"
)


add_executable(wrapper_test_7 example_app_3.c)
# the lib that contains dlopen
target_link_libraries(wrapper_test_7 PRIVATE dl)
add_dependencies(wrapper_test_7 wrapper_normal_no)
target_compile_definitions(wrapper_test_7 PUBLIC LIBRARY_TO_LOAD="$<TARGET_FILE:wrapper_modified_no>")
add_test(NAME wrapper_test_7 COMMAND wrapper_test_7)
set_tests_properties(wrapper_test_7 PROPERTIES
        PASS_REGULAR_EXPRESSION "MPI Initialized with wildcards DISabled"
)


add_executable(wrapper_test_8 example_app_3.c)
# the lib that contains dlopen
target_link_libraries(wrapper_test_8 PRIVATE dl)
add_dependencies(wrapper_test_8 wrapper_normal_no)
target_compile_definitions(wrapper_test_8 PUBLIC LIBRARY_TO_LOAD="$<TARGET_FILE:wrapper_modified_with>")
add_test(NAME wrapper_test_8 COMMAND wrapper_test_8)
set_tests_properties(wrapper_test_8 PROPERTIES
        PASS_REGULAR_EXPRESSION "MPI Initialized with wildcards ENabled"
)



