
# modified stub library for testing
add_library(modified_mpi SHARED modified_mpi/mpi_lib.c)
target_include_directories(modified_mpi PUBLIC ${PROGRAM_TRAITS_INCLUDE_DIR})
target_link_libraries(modified_mpi PRIVATE ancor_programm_traits)

# normal stub library
add_library(normal_mpi SHARED normal_mpi/mpi_lib.c)

# test case naming sceme:
# default or modified header
# default or modified Library
# uses wildcards
# D_D_N = default default, no wildcards
# M_M_W modified,modified, has wildcards

function(add_testcase_configuration TEST_NAME SRC_CODE HEADER_TO_INCLUDE LIB_TO_LINK TAG_TO_USE EXPECTED_TEST_RESULT)
    add_executable(${TEST_NAME} example_app_1.c)
    target_include_directories(${TEST_NAME} PUBLIC ${HEADER_TO_INCLUDE} ${PROGRAM_TRAITS_INCLUDE_DIR})
    target_link_libraries(${TEST_NAME} PRIVATE ${LIB_TO_LINK})
    target_compile_definitions(${TEST_NAME} PUBLIC TAG_TO_USE=${TAG_TO_USE})
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    set_tests_properties(${TEST_NAME} PROPERTIES
            REQUIRED_PASS_REGULAR_EXPRESSION ${EXPECTED_TEST_RESULT}
    )
endfunction()

add_testcase_configuration(M_M_N example_app_1.c modified_mpi modified_mpi 42 "MPI Initialized with wildcards DISabled")
add_testcase_configuration(M_M_W example_app_1.c modified_mpi modified_mpi MATCHING_WILDCARD "MPI Initialized with wildcards ENabled")
target_compile_definitions(M_M_W PUBLIC USE_WILDCARD)

# building with normal header but linking against modified library: expect wildcard to be always enabled, as lib cannot be sure
add_testcase_configuration(N_M_N example_app_1.c normal_mpi modified_mpi 42 "MPI Initialized with wildcards ENabled")
add_testcase_configuration(N_M_W example_app_1.c normal_mpi modified_mpi MATCHING_WILDCARD "MPI Initialized with wildcards ENabled")
# has no effect in normal header
#target_compile_definitions(N_M_W PUBLIC USE_WILDCARD)

add_testcase_configuration(N_N_N example_app_1.c normal_mpi normal_mpi 42 "Normal MPI always is using wildcards")
add_testcase_configuration(N_N_W example_app_1.c normal_mpi normal_mpi MATCHING_WILDCARD "Normal MPI always is using wildcards")
# has no effect in normal header
#target_compile_definitions(N_N_W PUBLIC USE_WILDCARD)


add_testcase_configuration(M_N_N example_app_1.c modified_mpi normal_mpi 42 "Normal MPI always is using wildcards")
add_testcase_configuration(M_N_W example_app_1.c modified_mpi normal_mpi MATCHING_WILDCARD "Normal MPI always is using wildcards")
target_compile_definitions(M_N_W PUBLIC USE_WILDCARD)



# test the new header without USE_WILDCARD fails compilation:
#expected to FAIL compilation
add_executable(M_M_W_fail example_app_1.c)
target_include_directories(M_M_W_fail PUBLIC modified_mpi  ${PROGRAM_TRAITS_INCLUDE_DIR})
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

# fail compilation
add_executable(M_N_W_fail example_app_1.c)
target_include_directories(M_N_W_fail PUBLIC modified_mpi ${PROGRAM_TRAITS_INCLUDE_DIR})
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
# add several configurations of the library:
add_library(wrapper_normal_no SHARED wrapper_lib/wrapper_lib.c)
target_include_directories(wrapper_normal_no PUBLIC normal_mpi)
target_link_libraries(wrapper_normal_no PRIVATE normal_mpi)

add_library(wrapper_modified_no SHARED wrapper_lib/wrapper_lib.c)
target_include_directories(wrapper_modified_no PUBLIC modified_mpi ${PROGRAM_TRAITS_INCLUDE_DIR})
target_link_libraries(wrapper_modified_no PRIVATE modified_mpi)

add_library(wrapper_normal_with SHARED wrapper_lib/wrapper_lib.c)
target_include_directories(wrapper_normal_with PUBLIC normal_mpi)
target_link_libraries(wrapper_normal_with PRIVATE normal_mpi)
# not needed with normal header
#target_compile_definitions(wrapper_normal_with PUBLIC USE_WILDCARD)
target_compile_definitions(wrapper_normal_with PUBLIC TAG_TO_USE=MATCHING_WILDCARD)

add_library(wrapper_modified_with SHARED wrapper_lib/wrapper_lib.c)
target_include_directories(wrapper_modified_with PUBLIC modified_mpi ${PROGRAM_TRAITS_INCLUDE_DIR})
target_link_libraries(wrapper_modified_with PRIVATE modified_mpi)
target_compile_definitions(wrapper_modified_with PUBLIC USE_WILDCARD)
target_compile_definitions(wrapper_modified_with PUBLIC TAG_TO_USE=MATCHING_WILDCARD)

add_library(wrapper_modified_with_fail SHARED wrapper_lib/wrapper_lib.c)
target_include_directories(wrapper_modified_with_fail PUBLIC modified_mpi ${PROGRAM_TRAITS_INCLUDE_DIR})
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


# test the SAME source code linked against the different version of this wrapper library
# note that the header of the Lib is always the same

function(add_simple_wrapper_testcase TEST_NAME SRC_CODE WRAPPER_LIB EXPECTED_TEST_RESULT)
    add_executable(${TEST_NAME} ${SRC_CODE})
    target_include_directories(${TEST_NAME} PUBLIC wrapper_lib ${PROGRAM_TRAITS_INCLUDE_DIR})
    target_link_libraries(${TEST_NAME} PRIVATE ${WRAPPER_LIB})
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    set_tests_properties(${TEST_NAME} PROPERTIES
            REQUIRED_PASS_REGULAR_EXPRESSION ${EXPECTED_TEST_RESULT}
    )
endfunction()


add_simple_wrapper_testcase(wrapper_test_1 example_app_2.c wrapper_normal_no "Normal MPI always is using wildcards")

add_simple_wrapper_testcase(wrapper_test_2 example_app_2.c wrapper_normal_with "Normal MPI always is using wildcards")

add_simple_wrapper_testcase(wrapper_test_3 example_app_2.c wrapper_modified_no "MPI Initialized with wildcards DISabled")

add_simple_wrapper_testcase(wrapper_test_4 example_app_2.c wrapper_modified_with "MPI Initialized with wildcards ENabled")


function(add_dynamic_wrapper_testcase TEST_NAME SRC_CODE WRAPPER_LIB EXPECTED_TEST_RESULT)
    add_executable(${TEST_NAME} ${SRC_CODE})
    target_include_directories(${TEST_NAME} PUBLIC wrapper_lib ${PROGRAM_TRAITS_INCLUDE_DIR})
    # the lib that contains dlopen
    target_link_libraries(${TEST_NAME} dl)
    target_compile_definitions(${TEST_NAME} PUBLIC LIBRARY_TO_LOAD="$<TARGET_FILE:${WRAPPER_LIB}>")
    # add dependency on the library, as it loaded dynamically
    add_dependencies(${TEST_NAME} ${WRAPPER_LIB})
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    set_tests_properties(${TEST_NAME} PROPERTIES
            REQUIRED_PASS_REGULAR_EXPRESSION ${EXPECTED_TEST_RESULT}
    )
endfunction()


add_dynamic_wrapper_testcase(wrapper_test_5 example_app_3.c wrapper_normal_no "Normal MPI always is using wildcards")

add_dynamic_wrapper_testcase(wrapper_test_6 example_app_3.c wrapper_normal_with "Normal MPI always is using wildcards")

# in this case dopen happens before the init to MPI
add_dynamic_wrapper_testcase(wrapper_test_7 example_app_3.c wrapper_modified_no "MPI Initialized with wildcards DISabled")
# in this case dopen happens before the init to MPI : wildcard usage was known from the beginning
add_dynamic_wrapper_testcase(wrapper_test_8 example_app_3.c wrapper_modified_with "MPI Initialized with wildcards ENabled")

# the example code 4 dlopens the wrapper library AFTER initializing MPI

function(add_dynamic_wrapper_testcase_2 TEST_NAME SRC_CODE MPI_HEADER MPI_LIB WRAPPER_LIB EXPECTED_TEST_RESULT)
    add_executable(${TEST_NAME} ${SRC_CODE})
    target_include_directories(${TEST_NAME} PUBLIC wrapper_lib)
    target_include_directories(${TEST_NAME} PUBLIC ${MPI_HEADER} ${PROGRAM_TRAITS_INCLUDE_DIR})
    # the lib that contains dlopen
    target_link_libraries(${TEST_NAME} dl)
    target_link_libraries(${TEST_NAME} ${MPI_LIB})
    target_compile_definitions(${TEST_NAME} PUBLIC LIBRARY_TO_LOAD="$<TARGET_FILE:${WRAPPER_LIB}>")
    add_dependencies(${TEST_NAME} ${WRAPPER_LIB})
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    set_tests_properties(${TEST_NAME} PROPERTIES
            REQUIRED_PASS_REGULAR_EXPRESSION ${EXPECTED_TEST_RESULT}
    )
endfunction()


add_dynamic_wrapper_testcase_2(wrapper_test_9 example_app_4.c normal_mpi normal_mpi wrapper_normal_no "Normal MPI always is using wildcards")
add_dynamic_wrapper_testcase_2(wrapper_test_10 example_app_4.c normal_mpi normal_mpi wrapper_normal_with "Normal MPI always is using wildcards")
#in this case: first the normal MPI is linked then the wrapper that is compiled with modified MPI
# expect usage of normal MPI
add_dynamic_wrapper_testcase_2(wrapper_test_11 example_app_4.c normal_mpi normal_mpi wrapper_modified_no "Normal MPI always is using wildcards")
add_dynamic_wrapper_testcase_2(wrapper_test_12 example_app_4.c normal_mpi normal_mpi wrapper_modified_with "Normal MPI always is using wildcards")

# cases compiled against normal header: expect that modified lib enables wildcard support
add_dynamic_wrapper_testcase_2(wrapper_test_13 example_app_4.c normal_mpi modified_mpi wrapper_normal_no "MPI Initialized with wildcards ENabled")
add_dynamic_wrapper_testcase_2(wrapper_test_14 example_app_4.c normal_mpi modified_mpi wrapper_normal_with "MPI Initialized with wildcards ENabled")
add_dynamic_wrapper_testcase_2(wrapper_test_15 example_app_4.c normal_mpi modified_mpi wrapper_modified_no "MPI Initialized with wildcards ENabled")
add_dynamic_wrapper_testcase_2(wrapper_test_16 example_app_4.c normal_mpi modified_mpi wrapper_modified_with "MPI Initialized with wildcards ENabled")

# dlopen is implemented in /lib/x86_64-linux-gnu/libdl.so.2

# cases with modified header
# mpi should abort if it detects that wildcards are/may needed after init
# these cases use wrapper library with normal header: thus need to assume wildcard usage and abort on detection of this
add_dynamic_wrapper_testcase_2(wrapper_test_17 example_app_4.c modified_mpi modified_mpi wrapper_normal_no "Trait violation found after dlopen")
add_dynamic_wrapper_testcase_2(wrapper_test_18 example_app_4.c modified_mpi modified_mpi wrapper_normal_with "Trait violation found after dlopen")
# expected to fail
set_tests_properties(wrapper_test_17 PROPERTIES WILL_FAIL TRUE)
set_tests_properties(wrapper_test_18 PROPERTIES WILL_FAIL TRUE)

# in this case the wrapper does not have wildcards, so this configuration is valid
add_dynamic_wrapper_testcase_2(wrapper_test_19 example_app_4.c modified_mpi modified_mpi wrapper_modified_no "MPI Initialized with wildcards DISabled")

# wildcards are needed, but this is only visible after MPI init. Abort
add_dynamic_wrapper_testcase_2(wrapper_test_20 example_app_4.c modified_mpi modified_mpi wrapper_modified_with "Trait violation found after dlopen")
# expected to fail
set_tests_properties(wrapper_test_20 PROPERTIES WILL_FAIL TRUE)

# and cases compiled against modified header but executed with normal mpi library
add_dynamic_wrapper_testcase_2(wrapper_test_21 example_app_4.c modified_mpi normal_mpi wrapper_normal_no "Normal MPI always is using wildcards")
add_dynamic_wrapper_testcase_2(wrapper_test_22 example_app_4.c modified_mpi normal_mpi wrapper_normal_with "Normal MPI always is using wildcards")
add_dynamic_wrapper_testcase_2(wrapper_test_23 example_app_4.c modified_mpi normal_mpi wrapper_modified_no "Normal MPI always is using wildcards")
add_dynamic_wrapper_testcase_2(wrapper_test_24 example_app_4.c modified_mpi normal_mpi wrapper_modified_with "Normal MPI always is using wildcards")
