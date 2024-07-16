This folder contains the remote control application that run on a windows laptop (and hopefully on a android phone eventually).


Howto build the client on Windows:
* Install MSYS2 (https://www.msys2.org/).
* Run the following command in the "MSYS2 UCRT64" terminal to install the required packages:
  * pacman -S base-devel mingw-w64-ucrt-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-ucrt-x86_64-gtk4-media-gstreamer glib2-devel mingw-w64-ucrt-x86_64-gst-plugins-good mingw-w64-ucrt-x86_64-gst-plugins-bad mingw-w64-ucrt-x86_64-gst-plugins-ugly mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-qt6-multimedia mingw-w64-ucrt-x86_64-boost mingw-w64-ucrt-x86_64-gst-libav

* In Visual Studio Code: 
  * Install the CMake extension
  * Set the "cmake.cmakePath" setting to "C:\msys64\mingw64\bin\cmake.exe"
  * Set the "cmake.sourceDirectory" setting to:
        "C:/Users/knut.johannessen/Private/snowrobot/remotecontrol2/client"
 
  * Restart Visual Studio Code.