cmake_policy(SET CMP0167 NEW)

project(sylar-src)

file(
  GLOB_RECURSE PROJECT_SOURCES
  ./*.cc
)

# message(STATUS "Project sources: ${PROJECT_SOURCES}")

file(
  GLOB_RECURSE CCLS_CACHE_FILES
  ./.ccls-cache/*.cc
)

file(
  GLOB_RECURSE SYLAR_TEST_FILES
  ./test/*.cc
  ./example/*.cc
)

list(REMOVE_ITEM PROJECT_SOURCES ${CCLS_CACHE_FILES})
list(REMOVE_ITEM PROJECT_SOURCES ${SYLAR_TEST_FILES})

add_library(${PROJECT_NAME} SHARED ${PROJECT_SOURCES})

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)
force_redefine_file_macro_for_sources(${PROJECT_NAME})

find_package(yaml-cpp REQUIRED)
find_package(Boost REQUIRED)
find_package(Threads REQUIRED)

set(LIBS
  yaml-cpp
  Boost::boost
  Threads::Threads
  dl
  )

target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBS})

add_subdirectory(test)
add_subdirectory(example)
