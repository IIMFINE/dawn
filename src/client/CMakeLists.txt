
add_compile_options(-Wall -g -std=c++17)
set(OUTPUT_EXEC_FILE client_app)
set(DIR_HEAD_FILE ${CMAKE_SOURCE_DIR}/include)            #set head file directory
set(DIR_SRC ${CMAKE_CURRENT_SOURCE_DIR})                            #set source file directory
set(DIR_COMMON_FILE ${CMAKE_SOURCE_DIR}/common)
set(ADD_THIRD_PARTY_LIB)
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "/usr/local/lib/cmake/log4cplus")

include_directories(${DIR_HEAD_FILE})                                   #add head file directory
aux_source_directory(${DIR_SRC} SRC_FILE)                           #add all the source file in the source file directory
aux_source_directory(${DIR_COMMON_FILE} COMMON_FILE)
add_executable(${OUTPUT_EXEC_FILE} ${SRC_FILE} ${COMMON_FILE})

#find log4cplus library
find_package(log4cplus)

if(log4cplus_FOUND)
    message(STATUS "[info]log4cplus library found")
    TARGET_INCLUDE_DIRECTORIES(${OUTPUT_EXEC_FILE} PUBLIC ${log4cplus_DIR})
    list(APPEND ADD_THIRD_PARTY_LIB "log4cplus")               #use find_package whill create the wrong ldd name,now I specify the log4cplus name to avoid this problem temporarily.
    add_definitions(-DUSE_LOG4CPLUS)
else()
    message(FATAL_ERROR "log4cplus library not found")
endif()

message(STATUS ${ADD_THIRD_PARTY_LIB})

TARGET_LINK_LIBRARIES(${OUTPUT_EXEC_FILE}
pthread
libmysqlclient.so
${ADD_THIRD_PARTY_LIB}
)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)                 #CMAKE_SOURCE_DIR is the root entrance CMakeLists.txt directory
 