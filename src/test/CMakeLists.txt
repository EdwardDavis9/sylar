set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/test)

# set(MAIN_TEST
#    # ./test_log.cc
#    # ./test_config.cc
#    # ./test_thread.cc
#    # ./test_util.cc
#    # ./test_fiber.cc
#    # ./test_scheduler.cc
#    # ./test_iomanager.cc
#    # ./test_hook.cc
#    # ./test_address.cc
#    # ./test_socket.cc
#    # ./test_bytearray.cc
#    # ./test_http.cc
#    # ./test_http_parser.cc
#    # ./test_tcp_server.cc
#    # ./test_http_server.cc
#    # ./test_http_servlet.cc
#    # ./test_uri.cc
#    # ./test_http_connection.cc
#    # ./test_daemon.cc
#    # ./test_env.cc
#    # ./test.cc # test load config
#    # ./test_application.cc
#    ./${PROJECT_TEST_NAME}.cc
#    # ./test_http_server.cc ## ab -n 1000000 -c 200 -k xxxxx
# )

set(PROJECT_TEST_NAME "test_http_connection")
set(MAIN_TEST
    ./${PROJECT_TEST_NAME}.cc
)

add_executable(${PROJECT_TEST_NAME} ${MAIN_TEST})

add_dependencies(${PROJECT_TEST_NAME} sylar-src)

target_link_libraries(${PROJECT_TEST_NAME} PRIVATE sylar-src)

# target_include_directories ( ${PROJECT_TEST_NAME} PRIVATE
# ${CMAKE_SOURCE_DIR}/include )

force_redefine_file_macro_for_sources(${PROJECT_TEST_NAME})

# add_test(NAME test COMMAND ${PROJECT_TEST_NAME})
add_test(NAME test COMMAND ${PROJECT_TEST_NAME} WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
