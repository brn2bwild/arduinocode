#include <Servo.h>

#define pinVelocidad  11
#define pinIzquierda  7
#define pinDerecha    8
#define entradaPot    A0

Servo direccion;

void setup() {
  pinMode(pinVelocidad, OUTPUT);
  pinMode(pinIzquierda, OUTPUT);
  pinMode(pinDerecha, OUTPUT);

  direccion.attach(5);
}

void loop() {
  int adc = analogRead(A0);
  int velocidad = map(adc, 0, 1023, 30, 255);
  analogWrite(pinVelocidad, velocidad);
  digitalWrite(pinIzquierda, HIGH);
  digitalWrite(pinDerecha, LOW);

  direccion.write(90);
}
