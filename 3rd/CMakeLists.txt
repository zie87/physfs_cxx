#configure directories
set (THIRD_PARTY_MODULE_PATH "${PROJECT_SOURCE_DIR}/3rd")

#configure directories
set (CATCH_MODULE_PATH "${THIRD_PARTY_MODULE_PATH}/Catch")
set (CATCH_INCLUDE_PATH "${CATCH_MODULE_PATH}/single_include/catch2")

#include custom cmake function
include ("${CATCH_MODULE_PATH}/contrib/ParseAndAddCatchTests.cmake")

#set variables for tests
set (TEST_THIRD_PARTY_INCLUDE_PATH  ${CATCH_INCLUDE_PATH})
set (TEST_THIRD_PARTY_INCLUDE_PATH  ${TEST_THIRD_PARTY_INCLUDE_PATH} PARENT_SCOPE)