add_compile_options(-Wall -g -std=c++17)

# set common type head file directory
set(DIR_COMMON_HEAD_FILE ${CMAKE_SOURCE_DIR}/include/common)

# set common type source file directory
set(DIR_COMMON_FILE ${CMAKE_CURRENT_SOURCE_DIR})

# add all common source files
aux_source_directory(${DIR_COMMON_FILE} COMMON_FILE)

if(NOT TARGET common)
  message(STATUS ${COMMON_FILE})
  add_library(common OBJECT ${COMMON_FILE})
  target_include_directories(common PUBLIC $<BUILD_INTERFACE:${DIR_COMMON_HEAD_FILE}> $<INSTALL_INTERFACE:common>)
endif()
