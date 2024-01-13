This folder contains scripts for starting a gstreamer server and client by invoking the gst-launch-1.0 commandline application.

These can be use to check that the hardware and network works properly if the c++ applications gives us trouble.


Observations about lag:

I measured the lag by having the rpi film a running stopwatch on the PC screen and then taking a photo of the gstreamer window
and the stopwatch on the PC's screen. The difference in time in the two windows give a good approximation of the total lag.

When the camera was moving a lot the lag was around 2000ms. When the camera was still and the area of the changing stopwatch
digits was small the lag was around 500ms. The framerate looked good in both cases (but I didn't measure it).