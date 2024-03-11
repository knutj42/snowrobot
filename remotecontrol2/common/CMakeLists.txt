cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(SnowRobotCommon)

add_library(snowrobotcommon 
  linebasedserver.cpp
  network.cpp
  )

target_compile_features(snowrobotcommon PUBLIC cxx_std_20)

find_package(Boost 1.84.0
             COMPONENTS log
             REQUIRED)

target_include_directories(snowrobotcommon PUBLIC
    )

target_link_directories(snowrobotcommon PUBLIC
)


target_link_libraries(snowrobotcommon PUBLIC
    Boost::log
    gstreamer-1.0
    glib-2.0
    pthread
)

if(CMAKE_HOST_WIN32)
target_link_libraries(snowrobotcommon PUBLIC
#  ws2_32 # windows only
#  wsock32  # windows only
)
endif()
