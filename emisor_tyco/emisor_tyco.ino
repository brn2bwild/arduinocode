#include <RH_ASK.h>
#include <SPI.h>

RH_ASK driver;

const int gatilloDer = A0;
const int gatilloIzq = A1;
const int pinLed = 13;

int gatDer = 0;
int gatIzq = 0;

void setup() {
  pinMode(pinLed, OUTPUT);
  Serial.begin(9600);
  if(driver.init())
    Serial.println("Inicilizacion fallida");
}

void loop() {
  uint8_t buf[2];
  uint8_t buflen = sizeof(buf);
  
  gatDer = analogRead(gatilloDer);
  gatIzq = analogRead(gatilloIzq);

  buf[0] = map(gatDer,3,810,0,255);
  buf[1] = map(gatIzq,3,780,0,255);

  Serial.print(buf[0]);
  Serial.println(buf[1]);
  driver.send(buf,buflen);
  driver.waitPacketSent();

}
