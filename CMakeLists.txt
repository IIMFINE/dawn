#root cmake file

cmake_minimum_required(VERSION 3.16)
project(DAWN)

add_compile_options(-Wall -g -std=c++17 -fomit-frame-pointer)

#for perf
add_compile_options(-fomit-frame-pointer)

# set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_BUILD_TYPE Release)

#set spdlog macro
if(NOT TARGET spdlog)
    find_package(spdlog REQUIRED)
    add_definitions(-DUSE_SPDLOG)
    message(STATUS "use spdlog")
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/common)
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/client)
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/server)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/discovery)
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/publisher)
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/subscription)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test)
