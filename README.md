# Snowrobot

This repository contains the software controlling the snowrobot. 


The end-goal is a robot that can clear all the snow with no human input or monitoring. We will probably never get all the way there, but there are lots of fun and useful intemediate steps to do:


## Implement manual remote control

The first step is to implement manual remote control with a low-latency video feed. This can be developed using a small indoor teleprecence robot.

1. Get a raspberry pi up and running with remote access (ssh, vnc and tailscale).

2. Use gstream on the pi to make the camera feed available on a http port and view it in a webbrowser on the laptop.
  * I found some working scripts at https://cgit.freedesktop.org/gstreamer/gst-plugins-good/plain/tests/examples/rtp/:
      gstreamer-scripts\client-H264-PCMA.bat
      gstreamer-scripts\server-H264.sh
    These stream both audio and video

3. Two c++ applications; one for the rpi and one for the laptop:
   The rpi application should listen on a tcp/ip port so that the laptop application can connect to it.
   The laptop application should connect to the rpi app and tell it to create a gstreamer bin that mimics the one that
   the script "gstreamer-scripts\server-H264.sh" creates.
   The laptop application should then create a gstreamer bin that mimics the one the script "gstreamer-scripts\client-H264-PCMA.bat" creates.
   The laptop application displays a GUI (written in Qt) that will eventually let the user monitor and manually control the robot.

4. Update the c++ application on the pi to control the robot (gpio, etc).

5. Update the laptop application to connect to the pi application and send messages to it. When the user presses the arrow-buttons the laptop application should send motor-control messages to the pi application.

6. Update the pi application to control lego motors (using the remeo ble module).

7. Build a legorobot that can drive around untethered (we probably need to buy a large usb powerbank). Not sure if we should use skid-stearing or steerable wheels. (The snowrobot will use skid-stearing)