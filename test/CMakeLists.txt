project(test_dawn)

# set head file directory
set(DIR_HEAD_FILE ${CMAKE_SOURCE_DIR}/src)
set(DIR_TEST_HEAD_FILE ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(PROJECT_SOURCE_FILE_PATH ${CMAKE_SOURCE_DIR}/src)

# set source file directory
set(DIR_SRC ${CMAKE_CURRENT_SOURCE_DIR})
find_package(GTest CONFIG REQUIRED)

set(ADD_THIRD_PARTY_LIB GTest::gtest)

file(GLOB_RECURSE TEST_CASE_SOURCE_FILES "${CMAKE_SOURCE_DIR}/src/*test*.cpp")

message(STATUS "test case source file")
message(STATUS ${TEST_CASE_SOURCE_FILES})

add_executable(
  test_dawn ${DIR_SRC}/test.cpp
  ${DIR_SRC}/test_impl.cpp
  ${DIR_SRC}/test_boost.cpp
  ${DIR_SRC}/test_comm_func.cpp
  ${DIR_SRC}/test_benchmark.cpp
  ${DIR_SRC}/test_shm_transport.cpp
  ${DIR_SRC}/test_timer.cpp
  ${DIR_SRC}/test_heap.cpp
  ${DIR_SRC}/test_inotify.cpp
  ${TEST_CASE_SOURCE_FILES}
)

target_compile_options(test_dawn PRIVATE -std=c++17 -g -O0 -Wall -Werror -Wno-unused-variable -Wno-unused-function)

add_dependencies(test_dawn dawn)

# add head file directory
target_include_directories(test_dawn PRIVATE ${DIR_HEAD_FILE} ${DIR_TEST_HEAD_FILE})

message(STATUS ${ADD_THIRD_PARTY_LIB})

SET(CMAKE_VERBOSE_MAKEFILE 0)

TARGET_LINK_LIBRARIES(test_dawn
  PRIVATE
  pthread
  rt
  stdc++fs
  dawn
  ${ADD_THIRD_PARTY_LIB}
) # CMAKE_SOURCE_DIR is the root entrance CMakeLists.txt directory
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/../bin)
