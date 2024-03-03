This folder contains the two application that makes up the remote control of the robot.

The server runs on the Raspberry Pi.

The client runs on a PC or smartphone

The server sends video and audio to the client by using the gstreamer library. 

The client connects to the server via a tcp/ip port that the server listen on. This connection uses
a simple line-based protocol where the messages are json encoded values, and is used to exchange
high-level information between the server and client.

When the client connects, the following happens:
 * Server: 
     * Send a list of the available cameras and the udp ports the client needs to connect to for each camera.
 * Client: 
     * Send a list of udp ports the server needs to connect to for each camera
     * Create the gstreamer pipeline and add the components for each of camera
 * Server:
     * Create the gstreamer pipeline and add the components for each of camera

When the client disconnects, the following happens:
  * Client:
     * Stop and delete the gstreamer pipeline
     * Remove the CameraView qt widgets
  * Server:
     * Stop and delete the gstreamer pipeline


The server only accepts one client connection at a time. If a new client tries to connect when
the server already has a client connection an error-message is written to the client and the
connection is closed.