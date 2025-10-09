project(example_echo_server)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/example)

set(
  MAIN_EXAMPLE
  ./echo_server.cc
  # ./echo_client.cc
  )

add_executable(${PROJECT_NAME} ${MAIN_EXAMPLE})

add_dependencies(${PROJECT_NAME} sylar-src)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
  sylar-src
)

# force_redefine_file_macro_for_sources(${PROJECT_NAME})

# add_test(NAME main_example COMMAND ${PROJECT_NAME})
