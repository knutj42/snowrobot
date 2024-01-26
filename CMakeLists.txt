cmake_minimum_required(VERSION 3.10)

project(SnowRobot)

set(CMAKE_CTEST_ARGUMENTS "--verbose")
include(CTest)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)


add_subdirectory("remotecontrol2/common")

add_subdirectory("remotecontrol2/server")

add_subdirectory("remotecontrol2/client")

add_subdirectory("remotecontrol2/tests")
