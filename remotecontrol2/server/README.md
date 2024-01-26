This folder contains the server application that runs on the Raspberry Pi.


# Overview
The application consists of the following main parts:

## TcpServer
This component listens on a tcp/ip socket and accepts new connections. It is implemented using the boost::asio library. The client application connects to this server to control the various aspects of the server application. 
A tcp connection is represented by an instance of the Client class. 

There can multiple Clients at the same time, but only the oldest Connection can activly control the robot and access the cameras. All other Clients are "read only" and can only get limited telemetry from the robot.


## MotorController
This component talks to the motor controller hardware via bluetooth. It handles the bluetooth connection, retries, etc.


## CameraController
This component keeps and updated a list of the available cameras (which can be added and removed at any time by plugging and unplugging usb webcams). It can create a gstreamer pipeline for each camera.
