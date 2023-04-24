set(_OBJECT_NAME discovery)
add_compile_options(-Wall -g -std=c++17)

# set common header file directory
set(DIR_HEAD_FILE ${CMAKE_SOURCE_DIR}/include)

set(DIR_DISCOVERY_SRC_FILE ${CMAKE_CURRENT_SOURCE_DIR})

aux_source_directory(${DIR_DISCOVERY_SRC_FILE} DISCOVERY_FILE)

if(NOT TARGET ${_OBJECT_NAME})
  message(STATUS ${DISCOVERY_FILE})
  message(STATUS ${DIR_DISCOVERY_HEAD_FILE})
  add_library(${_OBJECT_NAME} OBJECT ${DISCOVERY_FILE})
  add_dependencies(${_OBJECT_NAME} common)
  target_include_directories(${_OBJECT_NAME}
    PUBLIC $<BUILD_INTERFACE:${DIR_HEAD_FILE}> $<INSTALL_INTERFACE:include/discovery>)
endif()
