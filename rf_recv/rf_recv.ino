#include <RH_ASK.h>
#include <SPI.h>
#include <Servo.h>

#define pinLed1     13
#define pinServo    9

RH_ASK driver;
Servo servo;
void setup() {
  // put your setup code here, to run once:
  pinMode(pinLed1, OUTPUT);
  servo.attach(pinServo);
  Serial.begin(9600);
  if(!driver.init())
    Serial.println("Inicialiación fallida");
}

void loop() {
  // put your main code here, to run repeatedly:
  uint8_t buf[2];
  uint8_t buflen = sizeof(buf);
  if(driver.recv(buf, &buflen)){
    int i;
    if(buf[0] == 97){
      digitalWrite(pinLed1,HIGH);
      servo.write(buf[1]);
    }else if(buf[0] == 114){
      digitalWrite(pinLed1,HIGH);
      servo.write(buf[1]);
    }else if(buf[0] == 100){
      digitalWrite(pinLed1,LOW);
      servo.write(buf[1]);
    }
  }
}
