cmake_policy(SET CMP0167 NEW)

# project(sylar-src)
set(PROJECT_LIB_NAME sylar-src)

# 移除这种文件包含的形式 ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# # message(STATUS "Project sources: ${PROJECT_SOURCES}")
# file(
#   GLOB_RECURSE PROJECT_SOURCES
#   ./*.cc
# )
# file(
#   GLOB_RECURSE CCLS_CACHE_FILES
#   ./.ccls-cache/*.cc
# )
# file(
#   GLOB_RECURSE SYLAR_TEST_FILES
#   ./test/*.cc
#   ./example/*.cc
# )
# list(REMOVE_ITEM PROJECT_SOURCES ${CCLS_CACHE_FILES})
# list(REMOVE_ITEM PROJECT_SOURCES ${SYLAR_TEST_FILES})

aux_source_directory(. PROJECT_SOURCES) # 当前目录
# aux_source_directory(./subdir SUBDIR_SOURCES)       # 手动添加子目录
# # 1: list(REMOVE_ITEM) 移除指定文件(需要完整路径或相对路径)
# list(REMOVE_ITEM PROJECT_SOURCES
#     "${CMAKE_CURRENT_SOURCE_DIR}/temp.cc"
#     "${CMAKE_CURRENT_SOURCE_DIR}/old.cc"
# )
# # 2: 使用 list(FILTER) 配合正则表达式排除特定文件
# # 例如,排除所有以 "temp_" 开头的 .cc 文件
# list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*/temp_.*\\.cc$")


file(GLOB RL_CC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/rl/*.rl")
set(RL_CC_OUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/rl")
set(RL_CC_OUTPUTS "")

# ragelmaker(
# sylar/http/http11_parser.rl
# LIB_SRC
# ${CMAKE_CURRENT_SOURCE_DIR}/sylar/http
# )
#
# ragelmaker(
# sylar/http/httpclient_parser.rl
# LIB_SRC
# ${CMAKE_CURRENT_SOURCE_DIR}/sylar/http
# )

foreach(rl_file ${RL_CC_FILES})
    ragelmaker(${rl_file} RL_CC_OUTPUTS ${RL_CC_OUT_DIR})
endforeach()

add_library(
  ${PROJECT_LIB_NAME}
  SHARED
  ${PROJECT_SOURCES}
  ${RL_CC_OUTPUTS}
)

target_include_directories(
  ${PROJECT_LIB_NAME}
  PUBLIC
  ${CMAKE_SOURCE_DIR}/include
)
force_redefine_file_macro_for_sources(${PROJECT_LIB_NAME})

find_package(yaml-cpp REQUIRED)
find_package(Boost REQUIRED)
find_package(Threads REQUIRED)


# Protobuf
# find_package(Protobuf REQUIRED)
# if(Protobuf_FOUND)
#   include_directories(${Protobuf_INCLUDE_DIRS})
#   target_include_directories(
#     ${PROJECT_LIB_NAME}
#     PUBLIC
#     ${Protobuf_INCLUDE_DIRS}
#   )
# endif()

# PROTOBUF_GENERATE_CPP(PB_SRCS PF_HDRS protobuf/test.proto)
# list(APPEND LIB_SRC ${PB_SRCS})
# message(status $(LIB_SRC})
#${PROTOBUF_LIBRARIES})


set(LIBS
  yaml-cpp::yaml-cpp
  Boost::boost
  Threads::Threads
  ${CMAKE_DL_LIBS}
  )

target_link_libraries(${PROJECT_LIB_NAME} PUBLIC ${LIBS})

add_subdirectory(test)
add_subdirectory(example)
