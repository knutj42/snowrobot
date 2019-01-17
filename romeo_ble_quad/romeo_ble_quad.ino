#include "RomeoBLEQuadDrive-knutjmodified/Motor.h"


/*!
* @file RemeoBLEQuadDrive.ino
* @brief RemeoBLEQuadDrive.ino PID control system of DC motor
*
*  RemeoBLEQuadDrive.ino Use PID control 4 way DC motor direction and speed
* 
* @author linfeng(490289303@qq.com)
* @version  V1.0
* @date  2016-4-14
*/

#include "PID_v1.h"

Motor motor[4];

const int motorDirPin[4][2] = { //Forward, Backward
/*Motor-driven IO ports*/
  {8,23},
  {7,9},  
  {24,14},
  {4,25}
};



void setSpeed(int motorIndex, float speed) {
  if(speed > 1.0){
    speed = 1.0;
  }else if(speed < -1.0){
    speed = -1.0;
  }

  int intSpeed = (int)(speed*200);
  motor[motorIndex].setSpeed(intSpeed);
}


long lastDistance[4];

void setup( void )
{
  pinMode(13, OUTPUT);
  Serial1.begin(115200);
  
  for(int i=0;i<4;i++){
    motor[i].setPid(2, 7, 0);
    motor[i].setPin(motorDirPin[i][0], motorDirPin[i][1]);
    motor[i].setSampleTime(100);
    motor[i].setChannel(i);
    motor[i].ready();
    lastDistance[i] = motor[i].getDistance();
  }

  /*
  for(int i=0;i<4;i++){
    motor[i].setSpeed(600);
  }*/

    motor[0].setSpeed(0);
    motor[1].setSpeed(0);

  //setSpeedWithPWM(0, 1);
}


int ledState = LOW;
int currentCmd[10];
int currentCmdByteCount = 0;
unsigned long timeOfLastMovementMessage = 0;

unsigned long timeOfLastDebugOutput = 0;
unsigned long timeOfLastSpeedMod = 0;


union {
   byte asBytes[4];
   float asFloat;
} byteFloatUnion;


int targetSpeed = 0;

void loop( void )
{

/*  for(int i=0;i<4;i++){
    motor[i].setSpeed(0);
  }
*/

  for(int i = 0; i < 4; i++){
    motor[i].calibrate();
  } 

/*
  unsigned long currentTime = millis();
  unsigned long timeSinceLastDebugOutput = currentTime - timeOfLastDebugOutput;
  if (timeSinceLastDebugOutput >= 1000) {
    timeOfLastDebugOutput = currentTime;
    for(int i = 0; i < 2; i++){
      long distance = motor[i].getDistance();
      float rps = (1000.0 *  (float)(distance - lastDistance[i]) / timeSinceLastDebugOutput);
      lastDistance[i] = distance;
      Serial1.print("motor[");
      Serial1.print(i);
      Serial1.print("] speed:");
      Serial1.print(motor[i].getSpeed());
      Serial1.print(" targetSpeed:");
      Serial1.print(targetSpeed);
      Serial1.print(" distance:");
      Serial1.print(distance);
      Serial1.print(" rps:");
      Serial1.print(rps);
      Serial1.println();
    } 
  }
*/
/*
  unsigned long timeSinceLastSpeedMod = currentTime - timeOfLastSpeedMod;
  if (timeSinceLastSpeedMod > 3000) {
    targetSpeed = 200;
    for(int i = 0; i < 1; i++){
       motor[i].setSpeed(targetSpeed);
    }
    timeOfLastSpeedMod = currentTime;
  }
  
*/


  if (timeOfLastMovementMessage != 0) {
    unsigned long elapsedTimeSinceLastMovementMessage = millis() - timeOfLastMovementMessage;
    if(elapsedTimeSinceLastMovementMessage > 1000L) {
      // Something is wrong, so stop the motors
      setSpeed(0, 0);
      setSpeed(1, 0);
      //setSpeed(2, 0);
      //setSpeed(3, 0);
      timeOfLastMovementMessage = 0;
    }
  }
    
  while (Serial1.available()) {
    int theByte = Serial1.read();
    currentCmd[currentCmdByteCount] = theByte;
    currentCmdByteCount++;
    
    if (theByte == 0) {
      if (currentCmdByteCount > 2) {
        // This is the end-of-message marker, so parse the current message
        uint8_t responseCode = 201;
        int msgSequenceNr = currentCmd[1];
        int cmdId = currentCmd[2];
        if (cmdId == 102 && currentCmdByteCount < 16) {
          // the 102 (pid) command is a specialcase that can contain embedded zeros
          continue;
        }
        switch(cmdId) {
          case 100:
            // movement message
            if (currentCmdByteCount == 6) {
              timeOfLastMovementMessage = millis();
              int steeringByte = currentCmd[3];
              int throttleByte = currentCmd[4];
              float steering = (steeringByte - 128) / 127.0f;
              float throttle = (throttleByte - 128) / 127.0f;
              float leftMotorThrottle = max(-1.0f, min(1.0f, throttle + steering));
              float rightMotorThrottle = max(-1.0f, min(1.0f, throttle - steering));
              setSpeed(0, leftMotorThrottle);
              setSpeed(1, rightMotorThrottle);
              responseCode = 200;  // msg ack
           }
           break;
           
           case 101:
             // this is a ping message. We only need to acknowledge it.
             responseCode = 200;
             break;          

          case 102:
            // PID tuning message
            if (currentCmdByteCount == 16) {
              byteFloatUnion.asBytes[0] = currentCmd[3];
              byteFloatUnion.asBytes[1] = currentCmd[4];
              byteFloatUnion.asBytes[2] = currentCmd[5];
              byteFloatUnion.asBytes[3] = currentCmd[6];
              float kp = byteFloatUnion.asFloat;
              
              byteFloatUnion.asBytes[0] = currentCmd[7];
              byteFloatUnion.asBytes[1] = currentCmd[8];
              byteFloatUnion.asBytes[2] = currentCmd[9];
              byteFloatUnion.asBytes[3] = currentCmd[10];
              float ki = byteFloatUnion.asFloat;
              
              byteFloatUnion.asBytes[0] = currentCmd[11];
              byteFloatUnion.asBytes[1] = currentCmd[12];
              byteFloatUnion.asBytes[2] = currentCmd[13];
              byteFloatUnion.asBytes[3] = currentCmd[14];
              float kd = byteFloatUnion.asFloat;

              kp = 2.0;
              ki = 7.0;
              kd = 0.0;
              for(int i=0;i<4;i++){
                motor[i].setPid(kp, ki, kd);
              }

              responseCode = 200;  // msg ack
           }
           break;        
        }
        currentCmdByteCount = 0;
        Serial1.write((uint8_t)0);  // msg start marker
        Serial1.write(msgSequenceNr);
        Serial1.write(responseCode);
        Serial1.write((uint8_t)0);  // msg end marker
      }
    }
    
  }
  /*setSpeed(0, 0.0);
  setSpeed(1, 0.0);
  delay(1000);

  digitalWrite(13, LOW);
  setSpeed(0, 0.0);
  setSpeed(1, 0.0);
  delay(10000); */
}

