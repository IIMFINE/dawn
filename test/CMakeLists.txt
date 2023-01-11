cmake_minimum_required(VERSION 3.10)
add_compile_options(-Wall -g -std=c++17)
set(OUTPUT_EXEC_FILE test_app)
#set head file directory
set(DIR_HEAD_FILE ${CMAKE_SOURCE_DIR}/include)

#set source file directory
set(DIR_SRC ${CMAKE_CURRENT_SOURCE_DIR})
set(DIR_COMMON_FILE ${CMAKE_SOURCE_DIR}/common)
set(ADD_THIRD_PARTY_LIB)
#add all the source file in the source file directory
aux_source_directory(${DIR_SRC} SRC_FILE)
aux_source_directory(${DIR_COMMON_FILE} COMMON_FILE)
message(STATUS ${SRC_FILE})
message(STATUS ${COMMON_FILE})

if(NOT TARGET common_lib)
    add_library(common_lib OBJECT ${COMMON_FILE})
    target_include_directories(common_lib PUBLIC ${DIR_HEAD_FILE})
endif()

add_executable(${OUTPUT_EXEC_FILE} ${SRC_FILE} $<TARGET_OBJECTS:common_lib>)

#add head file directory
target_include_directories(${OUTPUT_EXEC_FILE} PRIVATE ${DIR_HEAD_FILE})

if(NOT TARGET spdlog)
    find_package(spdlog REQUIRED)
    add_definitions(-DUSE_SPDLOG)
    message(STATUS "spdlog found")
    list(APPEND ADD_THIRD_PARTY_LIB "spdlog::spdlog")
endif()

message(STATUS ${ADD_THIRD_PARTY_LIB})

TARGET_LINK_LIBRARIES(${OUTPUT_EXEC_FILE}
PRIVATE
pthread
${ADD_THIRD_PARTY_LIB}
)

#CMAKE_SOURCE_DIR is the root entrance CMakeLists.txt directory
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
