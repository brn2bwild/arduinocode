#include <RH_ASK.h>
#include <SPI.h>

#define pinLed    13
#define pinBoton1  5
#define pinBoton2  4


RH_ASK driver;
//Servo servo;

void setup() {
  // put your setup code here, to run once:
  pinMode(pinLed, OUTPUT);
  pinMode(pinBoton1, INPUT);
  pinMode(pinBoton2, INPUT);
  Serial.begin(9600);
  if(!driver.init())
    Serial.println("Inicialiación fallida");
}

void loop() {
  // put your main code here, to run repeatedly:
  if(digitalRead(pinBoton1)){
    const char *comando = "i";
    driver.send((uint8_t *)comando,strlen(comando));
    driver.waitPacketSent();
    digitalWrite(pinLed,HIGH);
    delay(2);
  }else if(digitalRead(pinBoton2)){
    const char *comando = "d";
    driver.send((uint8_t *)comando,strlen(comando));
    driver.waitPacketSent();
    digitalWrite(pinLed,HIGH);
    delay(2);
  }
  else{
    const char *comando = "c";
    driver.send((uint8_t *)comando,strlen(comando));
    driver.waitPacketSent();
    digitalWrite(pinLed,LOW);
    delay(2);
  }
}
