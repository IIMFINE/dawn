
add_compile_options(-Wall -g)
set(DIR_HEAD_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../../include)            #set head file directory
set(DIR_SRC ${CMAKE_CURRENT_SOURCE_DIR})                            #set source file directory
set(DIR_COMMON_FILE ${CMAKE_SOURCE_DIR}/common)
include_directories(${DIR_HEAD_FILE})                                   #add head file directory
aux_source_directory(${DIR_SRC} SRC_FILE)                           #add all the source file in the source file directory
aux_source_directory(${DIR_COMMON_FILE} COMMON_FILE)
add_executable(server_app ${SRC_FILE} ${COMMON_FILE})

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)                 #CMAKE_SOURCE_DIR is the root entrance CMakeLists.txt directory

set_target_properties(${TARGET} PROPERTIES
	COMPILE_FLAGS "-pthread"
	LINK_FLAGS "-pthread")