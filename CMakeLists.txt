#root cmake file

cmake_minimum_required(VERSION 3.10)

project(DAWN)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/client)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/server)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test)
