#include <RH_ASK.h>
#include <SPI.h>

const int pinLed=     13;
const int pinEn1=     6;
const int motorA1=    4;
const int motorA2=    5;
const int pinEn2=     9;
const int motorB1=    7;
const int motorB2=    8;

RH_ASK driver;

void setup() {
  pinMode(pinLed, OUTPUT);
  pinMode(pinEn1, OUTPUT);
  pinMode(pinEn2, OUTPUT);
  pinMode(motorA1, OUTPUT);
  pinMode(motorA2, OUTPUT);
  pinMode(motorB1, OUTPUT);
  pinMode(motorB2, OUTPUT);
  Serial.begin(9600);
  if(!driver.init())
    Serial.println("Inicialiación fallida");
  digitalWrite(pinEn1, HIGH);
  digitalWrite(pinEn2, HIGH);
}

void loop() {
  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
  uint8_t buflen = sizeof(buf);
  if(driver.recv(buf, &buflen)){
    int i;
    if(buf[0] > 180 && buf[1] > 180){
      digitalWrite(motorA1,HIGH);
      digitalWrite(motorA2,LOW);
      digitalWrite(motorB1,LOW);
      digitalWrite(motorB2,HIGH);
      digitalWrite(pinLed,HIGH);
    }else if(buf[0] < 60 && buf[1] < 60){
      digitalWrite(motorA1,LOW);
      digitalWrite(motorA2,HIGH);
      digitalWrite(motorB1,HIGH);
      digitalWrite(motorB2,LOW);  
      digitalWrite(pinLed,HIGH);
    }else if(buf[0] > 180 && buf[1] < 60){
      digitalWrite(motorA1,HIGH);
      digitalWrite(motorA2,LOW);
      digitalWrite(motorB1,HIGH);
      digitalWrite(motorB2,LOW);
      digitalWrite(pinLed,HIGH);
    }else if(buf[0] < 60 && buf[1] > 180){
      digitalWrite(motorA1,LOW);
      digitalWrite(motorA2,HIGH);
      digitalWrite(motorB1,LOW);
      digitalWrite(motorB2,HIGH);
      digitalWrite(pinLed,HIGH);
    }else{
      digitalWrite(motorA1,LOW);
      digitalWrite(motorA2,LOW);
      digitalWrite(motorB1,LOW);
      digitalWrite(motorB2,LOW);
      digitalWrite(pinLed,LOW);
    }
  }
}
