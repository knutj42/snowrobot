cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package (Python3 COMPONENTS Interpreter)



add_test(NAME integrationtests
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/run_tests.py
)

set_tests_properties(integrationtests
  PROPERTIES
    ENVIRONMENT "CMAKE_CURRENT_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR};CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}"
)
