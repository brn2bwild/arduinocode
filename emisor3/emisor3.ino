#include <RH_ASK.h>
#include <SPI.h>
#include <Servo.h>

RH_ASK driver;

const int gatilloDer = A0;
const int gatilloIzq = A1;
const int joystickX = A2;
const int pinLed = 13;

int gatDer = 512;
int gatIzq = 512;
int dirJoy = 512;
int resultado = 0;

void setup() {
  pinMode(pinLed, OUTPUT);
  Serial.begin(9600);
  if(!driver.init())
    Serial.println("Inicializacion fallida");

}

void loop() {
  uint8_t buf[3];
  uint8_t buflen = sizeof(buf);

  gatDer = analogRead(gatilloDer);
  gatIzq = analogRead(gatilloIzq);
  dirJoy = analogRead(joystickX);

  resultado = gatIzq - gatDer;
  if(resultado >= 0){
    buf[1] = map(resultado,0,500,0,255);
    buf[0] = 1;
  }
  else{
    buf[1] = map(resultado,0,-495,0,155);
    buf[0] = 0;
  }

  buf[2] = map(dirJoy,855,3,100,70);
  Serial.print(buf[0]);
  Serial.print(buf[1]);
  Serial.println(buf[2]);
  driver.send(buf,buflen);
  driver.waitPacketSent();
}
