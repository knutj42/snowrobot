cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(Server)

add_executable(server server.cpp)


target_compile_features(server PUBLIC cxx_std_20)

find_package(Boost 1.84.0
             COMPONENTS json log program_options
             REQUIRED)

if(CMAKE_HOST_WIN32)
target_include_directories(server PRIVATE
  C:/msys64/ucrt64/include/gstreamer-1.0
  C:/msys64/ucrt64/include/glib-2.0
  C:/msys64/usr/lib/glib-2.0/include
)
else()
target_include_directories(server PRIVATE
    /usr/include/gstreamer-1.0
    /usr/include/glib-2.0
    /usr/lib/arm-linux-gnueabihf/glib-2.0/include
)
endif()



target_link_directories(server PRIVATE
)


target_link_libraries(server PRIVATE
    snowrobotcommon
    Boost::json
    Boost::log
    Boost::program_options
    gstreamer-1.0
    glib-2.0
    gobject-2.0
    pthread

    #libgstapp-1.0

)

if(CMAKE_HOST_WIN32)
target_link_libraries(server PRIVATE
  ws2_32 # windows only
  wsock32  # windows only
)
endif()
