#include <RH_ASK.h>
#include <SPI.h>

#define pinLed          13
#define botonAdel       5
#define botonAtras      4
const int joystickPin = A0;
uint8_t joystickMap =   0;

RH_ASK driver;

void setup() {
  // put your setup code here, to run once:
  pinMode(pinLed, OUTPUT);
  pinMode(botonAdel, INPUT);
  pinMode(botonAtras, INPUT);
  Serial.begin(9600);
  if(!driver.init())
    Serial.println("Inicialiación fallida");
}

void loop() {
  joystickMap = map(analogRead(joystickPin),0,1023,75,115);

  if(digitalRead(botonAdel)){
    uint8_t buf[2] = {97,joystickMap};
    driver.send(buf,sizeof(buf));
    driver.waitPacketSent();
    digitalWrite(pinLed,HIGH);
    delay(1);
  }else if(digitalRead(botonAtras)){
    uint8_t buf[2] = {114,joystickMap};
    driver.send(buf,sizeof(buf));
    driver.waitPacketSent();
    digitalWrite(pinLed,HIGH);
    delay(1);
  }else{
    uint8_t buf[2] = {100,joystickMap};
    driver.send(buf,sizeof(buf));
    driver.waitPacketSent();
    digitalWrite(pinLed,LOW);
    delay(1);
  }
}
