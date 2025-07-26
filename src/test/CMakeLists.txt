project(test_iomanager)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/test)

set(
  MAIN
  # ./test.cc
  # ./test_thread.cc
  # ./test_util.cc
  # ./test_fiber.cc
  # ./test_scheduler.cc
  ./test_iomanager.cc
)

add_executable(${PROJECT_NAME} ${MAIN})

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
