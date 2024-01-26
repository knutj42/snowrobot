cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(Client)

add_executable(client client.cpp)
target_compile_features(client PUBLIC cxx_std_23)

find_package(Boost 1.84.0
             COMPONENTS log
             REQUIRED)

target_link_libraries(client PRIVATE
                      Boost::log)


target_include_directories(client PRIVATE
    C:/msys64/ucrt64/include/gstreamer-1.0
    C:/msys64/ucrt64/include/qt6
    C:/msys64/mingw64/include/glib-2.0
    C:/msys64/usr/lib/glib-2.0/include
    )

target_link_directories(client PRIVATE
    C:/msys64/ucrt64/lib
)


target_link_libraries(client PRIVATE
    snowrobotcommon
    Boost::log
    pthread
    libgstreamer-1.0
    libglib-2.0
    libQt6Core
    libQt6Gui
    libQt6Widgets
    #libgstapp-1.0

)



if(CMAKE_HOST_WIN32)
target_link_libraries(client PRIVATE
  ws2_32 # windows only
  wsock32  # windows only
)
endif()
