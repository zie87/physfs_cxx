cmake_minimum_required(VERSION 3.1)

project(physfs_cxx LANGUAGES C CXX VERSION 0.1.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wdeprecated-declarations")
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


option(USE_GCOV "Start coverage build" OFF)

find_package (ZLIB REQUIRED)

find_package (PhysFS REQUIRED)
include_directories(${PHYSFS_INCLUDE_DIR})

add_subdirectory (3rd EXCLUDE_FROM_ALL)

include_directories(include)
add_subdirectory(src)

find_package(Doxygen)
if(DOXYGEN_FOUND)
    add_subdirectory(doc)
endif()