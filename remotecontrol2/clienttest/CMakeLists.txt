cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(ClientTest)


add_executable(clienttest clienttest.cpp)
target_compile_features(clienttest PUBLIC cxx_std_23)


find_package(Boost 1.84.0
             COMPONENTS log program_options json
             REQUIRED)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network Gui Core MultimediaWidgets)

target_include_directories(clienttest PRIVATE
    C:/msys64/ucrt64/include/gstreamer-1.0
    C:/msys64/ucrt64/include/qt6
    C:/msys64/mingw64/include/glib-2.0
    C:/msys64/usr/lib/glib-2.0/include
    )

target_link_directories(clienttest PRIVATE
    C:/msys64/ucrt64/lib
)



target_link_libraries(clienttest PRIVATE
    snowrobotcommon
    Boost::json
    Boost::log
    Boost::program_options
    pthread
    gstreamer-1.0
    gstvideo-1.0
    glib-2.0
    gobject-2.0
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Widgets
    Qt6::MultimediaWidgets

)



if(CMAKE_HOST_WIN32)
target_link_libraries(clienttest PRIVATE
  ws2_32 # windows only
  wsock32  # windows only
)
endif()
