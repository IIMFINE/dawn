set(_OBJECT_NAME transport)

add_compile_options(-Wall -g -std=c++17)

set(DIR_HEAD_FILE ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE TRANSPORT_SRC_FILE "*.cpp")
list(FILTER TRANSPORT_SRC_FILE EXCLUDE REGEX ".*test.*")

if(NOT TARGET ${_OBJECT_NAME})
  message(STATUS "transport source files: ${TRANSPORT_SRC_FILE}")
  add_library(${_OBJECT_NAME} OBJECT ${TRANSPORT_SRC_FILE})
  add_dependencies(${_OBJECT_NAME} common)
  target_include_directories(${_OBJECT_NAME}
    PUBLIC $<BUILD_INTERFACE:${DIR_HEAD_FILE}> $<INSTALL_INTERFACE:include/transport> PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>)
endif()
