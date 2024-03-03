set(_OBJECT_NAME common)
add_compile_options(-Wall -g -std=c++17)

# set common type head file directory
set(DIR_COMMON_HEAD_FILE ${CMAKE_CURRENT_SOURCE_DIR})

# set common type source file directory
set(DIR_COMMON_FILE ${CMAKE_CURRENT_SOURCE_DIR})

# add all common source files
file(GLOB_RECURSE COMMON_CPP_FILES "*.cpp")
list(FILTER COMMON_CPP_FILES EXCLUDE REGEX ".*test.*")

if(NOT TARGET ${_OBJECT_NAME})
  message(STATUS "common source files: ${COMMON_CPP_FILES}")
  add_library(${_OBJECT_NAME} OBJECT ${COMMON_CPP_FILES})
  target_include_directories(${_OBJECT_NAME} PUBLIC $<BUILD_INTERFACE:${DIR_COMMON_HEAD_FILE}> $<INSTALL_INTERFACE:include/common>)
endif()
