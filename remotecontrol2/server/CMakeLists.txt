cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(Server)

add_executable(server server.cpp)

target_compile_features(server PUBLIC cxx_std_23)

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

if(CMAKE_HOST_WIN32)
target_link_libraries(server PRIVATE
  ws2_32 # windows only
  wsock32  # windows only
)
endif()

target_precompile_headers(server
  PRIVATE
    <chrono>
    <iostream>
    <memory>
    <typeinfo>

    <boost/asio/buffer.hpp>
    <boost/asio/detached.hpp>
    <boost/asio/experimental/awaitable_operators.hpp>
    <boost/asio/ip/tcp.hpp>
    <boost/asio/use_awaitable.hpp>
    <boost/asio/write.hpp>
    <boost/exception/diagnostic_information.hpp> 
)
