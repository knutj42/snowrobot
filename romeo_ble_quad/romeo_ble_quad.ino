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
int motorSpeed[4] = {0,0,0,0};/*Set 4 speed motor*/
/* Speed=motorSpeed/(32*(setSampleTime/1000))(r/s) */
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
  for(int i=0;i<4;i++){
    motor[i].setPid(1, 0, 0);/*Tuning PID parameters*/
    motor[i].setPin(motorDirPin[i][0],motorDirPin[i][1]);/*Configure IO ports*/
    motor[i].setChannel(i);
    motor[i].ready();
  }
}


void loop( void )
{
  digitalWrite(13, HIGH);
  setSpeed(0, 0.0);
  setSpeed(1, 0.0);
  delay(1000);

  digitalWrite(13, LOW);
  setSpeed(0, 0.0);
  setSpeed(1, 0.0);
  delay(10000); 
}

