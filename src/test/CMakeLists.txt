project(test_http_servlet)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/test)

set(
  MAIN_TEST
  # ./test.cc
  # ./test_thread.cc
  # ./test_util.cc
  # ./test_fiber.cc
  # ./test_scheduler.cc
  # ./test_iomanager.cc
  # ./test_hook.cc
  # ./test_address.cc
  # ./test_socket.cc
  # ./test_bytearray.cc
  # ./test_http.cc
  # ./test_http_parser.cc
  # ./test_TCP_server.cc
  # ./test_http_server.cc
  ./test_http_servlet.cc
)

add_executable(${PROJECT_NAME} ${MAIN_TEST})

add_dependencies(${PROJECT_NAME} sylar-src)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
  sylar-src
)

# target_include_directories (
#   ${PROJECT_NAME}
#   PRIVATE
#   ${CMAKE_SOURCE_DIR}/include
# )

force_redefine_file_macro_for_sources(${PROJECT_NAME})

add_test(NAME test COMMAND ${PROJECT_NAME})
