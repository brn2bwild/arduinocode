#include <Servo.h>

Servo ESC;

#define ejeX A0

void setup() {
  ESC.attach(9);
  calibrarMotor();
}

void loop() {
  int lectura = analogRead(ejeX);
  int velocidad = map(lectura, 0, 1024, 1000, 2000);
  ESC.writeMicroseconds(velocidad);
}

void calibrarMotor(){
  ESC.writeMicroseconds(1000);
  delay(5000);
}
