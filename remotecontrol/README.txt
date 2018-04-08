This folder contains the code for the various bits and pieces that make up the remote control system for the snowrobot.

High level description:
* The "brains" of the robot is an Android phone
* The robot hardware is controlled by an Arduino mega board. 
* The phone controls the Arduino via bluetooth
* The phone can be remote controlled from the webpage https://robots.knutj.org
  The webpage uses webrtc to get a videofeed from the phone.

