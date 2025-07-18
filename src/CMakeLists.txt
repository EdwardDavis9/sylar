cmake_policy(SET CMP0167 NEW)

project(sylar-src)
include (../cmake/utils.cmake)

file(
  GLOB_RECURSE PROJECT_SOURCES
  ./*.cc
)

file(
  GLOB_RECURSE CCLS_CACHE_FILES
  ./.ccls-cache/*.cc
)

file(
  GLOB_RECURSE SYLAR_TEST_FILES
  ./test/*.cc
)

list(REMOVE_ITEM PROJECT_SOURCES ${CCLS_CACHE_FILES})
list(REMOVE_ITEM PROJECT_SOURCES ${SYLAR_TEST_FILES})

add_library(${PROJECT_NAME} SHARED ${PROJECT_SOURCES})

target_include_directories(
  ${PROJECT_NAME}
  PRIVATE
  ${CMAKE_SOURCE_DIR}/include
)
force_redefine_file_macro_for_sources(${PROJECT_NAME})

find_package(yaml-cpp REQUIRED)
find_package(Boost REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC yaml-cpp Boost::boost)

add_subdirectory(test)
