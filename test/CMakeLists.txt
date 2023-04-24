project(test)

#set head file directory
set(DIR_HEAD_FILE ${CMAKE_SOURCE_DIR}/include)
set(DIR_TEST_HEAD_FILE ${CMAKE_CURRENT_SOURCE_DIR}/include)

#set source file directory
set(DIR_SRC ${CMAKE_CURRENT_SOURCE_DIR})
find_package(GTest CONFIG REQUIRED)

set(ADD_THIRD_PARTY_LIB GTest::gtest)

add_executable(test ${DIR_SRC}/test.cpp ${DIR_SRC}/test_impl.cpp ${DIR_SRC}/test_boost.cpp)

add_dependencies(test common discovery transport)

#add head file directory
target_include_directories(test PRIVATE ${DIR_HEAD_FILE} ${DIR_TEST_HEAD_FILE})

message(STATUS ${ADD_THIRD_PARTY_LIB})

SET (CMAKE_VERBOSE_MAKEFILE 0)

TARGET_LINK_LIBRARIES(test
  PRIVATE
  $<TARGET_OBJECTS:common>
  $<TARGET_OBJECTS:discovery>
  $<TARGET_OBJECTS:transport>
  pthread
  rt
  ${ADD_THIRD_PARTY_LIB}
) # CMAKE_SOURCE_DIR is the root entrance CMakeLists.txt directory
set(EXECUTABLE_OUTPUT_PATH    ${CMAKE_SOURCE_DIR}/bin)


