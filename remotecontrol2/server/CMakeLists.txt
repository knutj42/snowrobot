cmake_minimum_required(VERSION 3.10)

project(Server)

add_executable(server server.cpp)
target_include_directories(server PRIVATE
    )

target_link_directories(server PRIVATE
)


target_link_libraries(server PRIVATE
    #gstreamer-1.0
    gstreamer-1.0
    glib-2.0
    pthread
    #libgstapp-1.0

)


target_precompile_headers(server
  PRIVATE <boost/asio.hpp>)
  
  
  
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
