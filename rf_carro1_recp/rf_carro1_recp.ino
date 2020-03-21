#include <RH_ASK.h>

RH_ASK controlador;

void setup() {
  pinMode(6, OUTPUT);
  Serial.begin(9600);
  if(!controlador.init())
    Serial.println("Inicialiación fallida");
}

void loop(){
  uint8_t buf[2];
  uint8_t buflen = sizeof(buf);
  if(controlador.recv(buf, &buflen)){
    analogWrite(6, buf[1]);
  }  
}
