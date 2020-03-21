#include <RH_ASK.h>
#include <SPI.h>
#include <Servo.h>

RH_ASK driver;

const int pinX = A0;
const int pinY = A1;

int joystickX = 512;
int joystickY = 512;
uint8_t joystickMap1 = 127;
uint8_t joystickMap2 = 127;

void setup(){
  pinMode(13,OUTPUT);
  Serial.begin(9600);
  if(!driver.init())
  Serial.println("Inicialiación fallida");
}
 
void loop(){
  joystickX = analogRead(pinX);
  joystickY = analogRead(pinY);
  joystickMap1 = map(joystickX,0,1023,100,70);
  joystickMap2 = map(joystickY,0,1023,0,255);
  
  uint8_t buf[2] = {joystickMap1,joystickMap2};
  uint8_t buflen = sizeof(buf);
  driver.send(buf,sizeof(buf));
  driver.waitPacketSent();
}

