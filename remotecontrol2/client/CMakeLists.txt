cmake_minimum_required(VERSION 3.10)

project(Client)

add_executable(client client.cpp)
target_include_directories(client PRIVATE
    C:/msys64/ucrt64/include/gstreamer-1.0
    C:/msys64/mingw64/include/glib-2.0
    C:/msys64/usr/lib/glib-2.0/include
    )

target_link_directories(client PRIVATE
    C:/msys64/ucrt64/lib
)


target_link_libraries(client PRIVATE
    #gstreamer-1.0
    libgstreamer-1.0
    libglib-2.0
    #libgstapp-1.0

)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)