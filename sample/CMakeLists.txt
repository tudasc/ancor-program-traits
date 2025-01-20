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
add_executable(M_M_N example_app.c)
target_include_directories(M_M_N PUBLIC modified_mpi)
target_link_libraries(M_M_N PRIVATE modified_mpi)
add_test(NAME M_M_N COMMAND M_M_N)
set_tests_properties(M_M_N PROPERTIES
        PASS_REGULAR_EXPRESSION "MPI Initialized with wildcards DISabled"
)

# expected to work with wildcards enabled
add_executable(M_M_W example_app.c)
target_include_directories(M_M_W PUBLIC modified_mpi)
target_link_libraries(M_M_W PRIVATE modified_mpi)
target_compile_definitions(M_M_W PUBLIC USE_WILDCARD)
target_compile_definitions(M_M_W PUBLIC TAG_TO_USE=MATCHING_WILDCARD)
add_test(NAME M_M_W COMMAND M_M_W)
set_tests_properties(M_M_W PROPERTIES
        PASS_REGULAR_EXPRESSION "MPI Initialized with wildcards ENabled"
)


#expected to FAIL compilation
add_executable(M_M_W_fail example_app.c)
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


add_executable(N_M_N example_app.c)
target_include_directories(N_M_N PUBLIC normal_mpi)
target_link_libraries(N_M_N PRIVATE modified_mpi)
add_test(NAME N_M_N COMMAND N_M_N)
# if compiled with normal header: marker is not present, need to assume wildcard usage
set_tests_properties(N_M_N PROPERTIES
        PASS_REGULAR_EXPRESSION "MPI Initialized with wildcards ENabled"
)

add_executable(N_M_W example_app.c)
target_include_directories(N_M_W PUBLIC normal_mpi)
target_link_libraries(N_M_W PRIVATE modified_mpi)
# has no effect in normal header
#target_compile_definitions(N_M_W PUBLIC USE_WILDCARD)
target_compile_definitions(N_M_W PUBLIC TAG_TO_USE=MATCHING_WILDCARD)
add_test(NAME N_M_W COMMAND N_M_W)
set_tests_properties(N_M_W PROPERTIES
        PASS_REGULAR_EXPRESSION "MPI Initialized with wildcards ENabled"
)



add_executable(N_N_N example_app.c)
target_include_directories(N_N_N PUBLIC normal_mpi)
target_link_libraries(N_N_N PRIVATE normal_mpi)
add_test(NAME N_N_N COMMAND N_N_N)
set_tests_properties(N_N_N PROPERTIES
        PASS_REGULAR_EXPRESSION "Normal MPI always is using wildcards"
)

add_executable(N_N_W example_app.c)
target_include_directories(N_N_W PUBLIC normal_mpi)
target_link_libraries(N_N_W PRIVATE normal_mpi)
# has no effect in normal header
#target_compile_definitions(N_N_W PUBLIC USE_WILDCARD)
target_compile_definitions(N_N_W PUBLIC TAG_TO_USE=MATCHING_WILDCARD)
add_test(NAME N_N_W COMMAND N_N_W)
set_tests_properties(N_N_W PROPERTIES
        PASS_REGULAR_EXPRESSION "Normal MPI always is using wildcards"
)


add_executable(M_N_N example_app.c)
target_include_directories(M_N_N PUBLIC modified_mpi)
target_link_libraries(M_N_N PRIVATE normal_mpi)
add_test(NAME M_N_N COMMAND M_N_N)
set_tests_properties(M_N_N PROPERTIES
        PASS_REGULAR_EXPRESSION "Normal MPI always is using wildcards"
)

add_executable(M_N_W example_app.c)
target_include_directories(M_N_W PUBLIC modified_mpi)
target_link_libraries(M_N_W PRIVATE normal_mpi)
target_compile_definitions(M_N_W PUBLIC USE_WILDCARD)
target_compile_definitions(M_N_W PUBLIC TAG_TO_USE=MATCHING_WILDCARD)
add_test(NAME M_N_W COMMAND M_N_W)
set_tests_properties(M_N_W PROPERTIES
        PASS_REGULAR_EXPRESSION "Normal MPI always is using wildcards"
)

# fail compilation
add_executable(M_N_W_fail example_app.c)
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