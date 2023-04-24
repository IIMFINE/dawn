set(_OBJECT_NAME transport)

add_compile_options(-Wall -g -std=c++17)

set(DIR_HEAD_FILE ${CMAKE_SOURCE_DIR}/include/transport)

set(DIR_SRC_FILE ${CMAKE_CURRENT_SOURCE_DIR})

aux_source_directory(${DIR_SRC_FILE} SRC_FILE)

if(NOT TARGET ${_OBJECT_NAME})
  message(STATUS ${SRC_FILE})
  message(STATUS ${DIR_HEAD_FILE})
  add_library(${_OBJECT_NAME} OBJECT ${SRC_FILE})
  add_dependencies(${_OBJECT_NAME} common)
  target_include_directories(${_OBJECT_NAME}
    PUBLIC $<BUILD_INTERFACE:${DIR_HEAD_FILE}> $<INSTALL_INTERFACE:include/transport> PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>)
endif()
