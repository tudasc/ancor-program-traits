add_executable(evaluation_driver evaluation_testcode.c)
target_include_directories(evaluation_driver PUBLIC ${PROGRAM_TRAITS_INCLUDE_DIR})
target_link_libraries(evaluation_driver PRIVATE ancor_programm_traits)


target_link_libraries(evaluation_driver PRIVATE m)

configure_file(generate_test_code.py generate_test_code.py @ONLY)
configure_file(evaluation_runner.py evaluation_runner.py @ONLY)
configure_file(visualize_results.py visualize_results.py COPYONLY)
