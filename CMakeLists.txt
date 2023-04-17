# root cmake file
cmake_minimum_required(VERSION 3.16)
project(dawn VERSION 0.1.0)

# support output symbol, c++17 and perf.
add_compile_options(-Wall -g -fomit-frame-pointer)
set(CMAKE_CXX_STANDARD 17)

# For build share library
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_BUILD_TYPE RelWithDebInfo)

# set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_BUILD_TYPE Release)

# set spdlog macro
if(NOT TARGET spdlog)
  find_package(spdlog REQUIRED)
  add_definitions(-DUSE_SPDLOG)
  message(STATUS "use spdlog")
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/common)

# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/client)
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/server)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/discovery)

# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/publisher)
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src/subscription)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test)

# ****************** install dawn library ******************
set(lib_name ${PROJECT_NAME})
set(export_name export_dawn)

# create export library
add_library(${lib_name} SHARED $<TARGET_OBJECTS:common> $<TARGET_OBJECTS:discovery>)

TARGET_LINK_LIBRARIES(${lib_name} PUBLIC pthread rt)
# create version file
include(CMakePackageConfigHelpers)

write_basic_package_version_file(
  ${lib_name}-version.cmake
  VERSION ${PACKAGE_VERSION}
  COMPATIBILITY AnyNewerVersion
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
  DESTINATION include/dawn
)

install(TARGETS ${lib_name}
  EXPORT ${export_name}
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  RUNTIME DESTINATION bin
  INCLUDES DESTINATION include
)

install(EXPORT ${export_name}
  FILE ${PROJECT_NAME}-target.cmake
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION lib/cmake/${PROJECT_NAME}
)

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/${lib_name}-config.cmake
  INSTALL_DESTINATION lib/cmake/${PROJECT_NAME}
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${lib_name}-config.cmake" "${CMAKE_CURRENT_BINARY_DIR}/${lib_name}-version.cmake"
  DESTINATION lib/cmake/${PROJECT_NAME}
)
