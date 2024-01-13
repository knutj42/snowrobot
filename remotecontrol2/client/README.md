This folder contains the remote control application that run on a windows laptop (and hopefully on a android phone eventually).


Howto build the client on Windows:
* Install MSYS2 (https://www.msys2.org/).
* Run the following command in the "MSYS2 UCRT64" terminal to install the required packages:
  * pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-ucrt-x86_64-gtk4-media-gstreamer glib2-devel mingw-w64-ucrt-x86_64-gst-plugins-good mingw-w64-ucrt-x86_64-gst-plugins-bad mingw-w64-ucrt-x86_64-gst-plugins-ugly

In the 