project(discovery)
add_compile_options(-Wall -g -std=c++17)
#set common header file directory
set(DIR_HEAD_FILE ${CMAKE_SOURCE_DIR}/include)

set(DIR_DISCOVERY_SRC_FILE ${CMAKE_CURRENT_SOURCE_DIR})

aux_source_directory(${DIR_DISCOVERY_SRC_FILE} DISCOVERY_FILE)

if(NOT TARGET discovery_object)
    message(STATUS ${DISCOVERY_FILE})
    message(STATUS ${DIR_DISCOVERY_HEAD_FILE})
    add_library(discovery_object OBJECT ${DISCOVERY_FILE})
    add_dependencies(discovery_object common)
    target_include_directories(discovery_object 
        PUBLIC $<BUILD_INTERFACE:${DIR_HEAD_FILE}> $<INSTALL_INTERFACE:include/discovery>)
endif()
