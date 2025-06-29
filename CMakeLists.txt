cmake_minimum_required(VERSION 3.20)
cmake_policy(SET CMP0167 NEW)

project( sylar-test VERSION 1.0 )

# 允许ccls能否正常索引头文件之类的
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_STANDARD 11)
# set(CMAKE_CXX_STANDARD_REQUIRED YES)
# set(CMAKE_CXX_EXTENSIONS NO)

# 可选：将所有编译输出放入单独目录中
# set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_BUILD_TYPE "Debug")
# set(CMAKE_BUILD_TYPE "Release")

# 配置 C 和 C++ 编译选项
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O0 -g -Wno-deprecated -Werror -Wno-unused-function")
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -Wno-deprecated -Werror -Wno-unused-function")
endif()


set(LIB_SRC src/log.cc)
add_library(sylar SHARED ${LIB_SRC})
target_include_directories(sylar PRIVATE ${CMAKE_SOURCE_DIR}/src)

file(GLOB_RECURSE PROJECT_SOURCES
  # ${CMAKE_SOURCE_DIR}/src/*.cpp
  # ${CMAKE_SOURCE_DIR}/src/*.c
  ${CMAKE_SOURCE_DIR}/src/*.cc

  # ${CMAKE_SOURCE_DIR}/src/test.cc

)

# 排除不必要的.ccls-cache目录
# file(GLOB_RECURSE REMOVE_CCLS_FILES "${CMAKE_SOURCE_DIR}/.ccls-cache/*")
# list(REMOVE_ITEM PROJECT_SOURCES ${REMOVE_CCLS_FILES})

# 排除不必要的 CMAKE 生成的文件
# file(GLOB_RECURSE REMOVE_CMAKE_FILES "${CMAKE_BINARY_DIR}/CMakeFiles/*")
# list(REMOVE_ITEM PROJECT_SOURCES ${REMOVE_CMAKE_FILES})

# 定义目标可执行文件
add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})


# 查找头文件
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)

add_dependencies(${PROJECT_NAME} sylar)

# 查找线程库
# find_package(Threads REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(Boost REQUIRED)

# target_link_libraries(${PROJECT_NAME} sylar)

target_link_libraries(${PROJECT_NAME} PRIVATE sylar yaml-cpp Boost::boost)

# 可选：安装规则（根据需要安装目标）
# install(TARGETS ${PROJECT_NAME}
#   RUNTIME DESTINATION bin
#   LIBRARY DESTINATION lib
#   ARCHIVE DESTINATION lib
# )

# 可选：启用测试支持
# enable_testing()
# add_subdirectory(tests)  # 如果有单元测试，可以将测试代码放在 tests 目录
