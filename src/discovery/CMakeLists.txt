set(_OBJECT_NAME discovery)
add_compile_options(-Wall -g -std=c++17)

# set common header file directory
set(DIR_HEAD_FILE ${CMAKE_CURRENT_SOURCE_DIR})

set(DIR_DISCOVERY_SRC_FILE ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE DISCOVERY_FILES "*.cpp")
list(FILTER DISCOVERY_FILES EXCLUDE REGEX ".*test.*")

if(NOT TARGET ${_OBJECT_NAME})
  message(STATUS "discovery source files: ${DISCOVERY_FILES}")
  add_library(${_OBJECT_NAME} OBJECT ${DISCOVERY_FILES})
  add_dependencies(${_OBJECT_NAME} common)
  target_include_directories(${_OBJECT_NAME}
    PUBLIC $<BUILD_INTERFACE:${DIR_HEAD_FILE}> $<INSTALL_INTERFACE:include/discovery> PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>)
endif()
