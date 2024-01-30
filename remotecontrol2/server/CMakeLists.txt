cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(Server)

add_executable(server server.cpp)


target_compile_features(server PUBLIC cxx_std_23)

find_package(Boost 1.84.0
             COMPONENTS log program_options
             REQUIRED)


target_include_directories(server PRIVATE
  C:/msys64/ucrt64/include/gstreamer-1.0
  C:/msys64/ucrt64/include/glib-2.0
  C:/msys64/usr/lib/glib-2.0/include
)

target_link_directories(server PRIVATE
)


target_link_libraries(server PRIVATE
    snowrobotcommon
    Boost::log
    Boost::program_options
    gstreamer-1.0
    glib-2.0
    pthread

    #libgstapp-1.0

)

if(CMAKE_HOST_WIN32)
target_link_libraries(server PRIVATE
  ws2_32 # windows only
  wsock32  # windows only
)
endif()
