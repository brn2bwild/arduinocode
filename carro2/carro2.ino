#include <RH_ASK.h>
#include <SPI.h>

RH_ASK driver;

const int pinLed = 13;
const int pinAdeIz = 7;
const int pinAtrIz = 6;
const int pinAdeDe = 5;
const int pinAtrDe = 4;

void setup(){
  pinMode(13,OUTPUT);
  Serial.begin(9600);
  if(!driver.init())
  Serial.println("Inicialiación fallida");
}
 
void loop(){
  uint8_t buf[2];
  uint8_t buflen = sizeof(buf);

  if(!digitalRead(pinAdeIz)){
    buf[0] = 'a';
    digitalWrite(pinLed,HIGH);
  }else if(!digitalRead(pinAtrIz)){
    buf[0] = 'r';
    digitalWrite(pinLed,HIGH);
  }else{
    buf[0] = 'd';
    digitalWrite(pinLed,LOW);
  }
  if(!digitalRead(pinAdeDe)){
    buf[1] = 'a';
    digitalWrite(pinLed,HIGH);
  }else if(!digitalRead(pinAtrDe)){
    buf[1] = 'r';
    digitalWrite(pinLed,HIGH);
  }else{
    buf[1] = 'd';
    digitalWrite(pinLed,LOW);
  }
  driver.send(buf,sizeof(buf));
  driver.waitPacketSent();    
}
