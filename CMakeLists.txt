cmake_minimum_required(VERSION 3.1)

project(physfs_cxx LANGUAGES C CXX VERSION 0.1.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wdeprecated-declarations")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(USE_GCOV "Start coverage build" OFF)

if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_BUILD_TYPE STREQUAL "Debug" AND USE_GCOV)
  include(CodeCoverage)
  set(CMAKE_CXX_FLAGS "-g -O0 -Wall -Wextra -Wdeprecated-declarations --coverage")
  set(LCOV_REMOVE_EXTRA '${PROJECT_SOURCE_DIR}/3rd/*' '${PROJECT_SOURCE_DIR}/src/tests/*')
endif()

find_package (PhysFS REQUIRED)
include_directories(${PHYSFS_INCLUDE_DIR})

include_directories(inc)

enable_testing()
add_subdirectory(src)

find_package(Doxygen)
if(DOXYGEN_FOUND)
    add_subdirectory(doc)
endif()