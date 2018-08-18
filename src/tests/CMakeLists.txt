set(CATCH_DIR ${CMAKE_CURRENT_SOURCE_DIR}/catch)
set(CATCH_MAIN_FILE ${CATCH_DIR}/catch_main.cxx)

include_directories(${CATCH_DIR})

set(PROJECT_TEST_NAME ${PROJECT_NAME}_test)
set(PROJECT_TEST_LIBS ${PHYSFS_LIBRARY})

add_definitions(-DTEST_DATA="${CMAKE_CURRENT_SOURCE_DIR}/data")

include(ParseAndAddCatchTests)

if (CMAKE_COMPILER_IS_GNUCXX AND CMAKE_BUILD_TYPE STREQUAL "Debug" AND USE_GCOV)
  setup_target_for_coverage(coverage ${PROJECT_TEST_NAME} coverage)
endif ()

add_executable(${PROJECT_TEST_NAME} "${CATCH_MAIN_FILE}"
                                    
                                    "${CMAKE_CURRENT_SOURCE_DIR}/physfs_cxx/basic_tests.cxx"
                                    "${CMAKE_CURRENT_SOURCE_DIR}/physfs_cxx/stream_tests.cxx"
)
target_link_libraries(${PROJECT_TEST_NAME} ${PROJECT_TEST_LIBS})

#parse catch tests
ParseAndAddCatchTests (${PROJECT_TEST_NAME})