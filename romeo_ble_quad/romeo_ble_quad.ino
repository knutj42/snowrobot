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
#include "Motor.h"

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

  // The actual value of the intSpeed doesn't matter, since we will set the raw PWM value later.
  // The only important thing is if the speed is positive or negative, since that specified the
  // direction of the motor.
  int intSpeed = (int)(speed*1000);
  motor[motorIndex].setSpeed(intSpeed);

  unsigned char pwmValue = (unsigned char)(abs(speed)*255);
  
  motor[motorIndex].PWMSet(pwmValue);

//  Serial1.write("pwmValue:");
  //Serial1.println((int)pwmValue);
  
}


void setup( void )
{
  pinMode(13, OUTPUT);
  Serial1.begin(115200);
  
  // Note that we don't actually use the PID-functionality in the Motor library, since we don't have 
  // encoders on the physical motors. TODO: figure out how to do this without the Motor library (analogWrite() 
  // doesn't seem to work for some reason).
  for(int i=0;i<4;i++){
    motor[i].setPid(1, 0, 0);
    motor[i].setPin(motorDirPin[i][0], motorDirPin[i][1]);
    motor[i].setChannel(i);
    motor[i].ready();
  }
}


int ledState = LOW;
int currentCmd[10];
int currentCmdByteCount = 0;
unsigned long timeOfLastMovementMessage = 0;

void loop( void )
{
  if (timeOfLastMovementMessage != 0) {
    unsigned long elapsedTimeSinceLastMovementMessage = millis() - timeOfLastMovementMessage;
    if(elapsedTimeSinceLastMovementMessage > 1000) {
      // Something is wrong, so stop the motors
      setSpeed(0, 0);
      setSpeed(1, 0);
      setSpeed(2, 0);
      setSpeed(3, 0);
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
        uint8_t responseCode = 500;
        int msgSequenceNr = currentCmd[1];
        int cmdId = currentCmd[2];
        if (cmdId == 100 && currentCmdByteCount == 6) {
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

