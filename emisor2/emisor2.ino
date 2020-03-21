#include <RH_ASK.h>

RH_ASK controlador;

void setup(){
  pinMode(A0, INPUT);
  Serial.begin(9600);
  if(!controlador.init())
    Serial.println("Inicialiación fallida");
}
void loop(){
  int valores[2];
  valores[1] = analogRead(A0);
  int velocidad = map(valores[1], 0, 1023, 0, 255);
  Serial.println(velocidad);

  controlador.send((uint8_t *)valores, sizeof(valores));
  controlador.waitPacketSent();
  delay(100);





  
//  uint8_t buf[3];
//  uint8_t buflen = sizeof(buf);
//  
//  joystickIzq = analogRead(izq);
//  joystickDer = analogRead(der);
//  
//  if(joystickIzq < 250){buf[0]=114;}
//  else if(joystickIzq > 700){buf[0]=97;}
//  else if(joystickIzq > 250 && joystickIzq < 700){buf[0]=100;}
//  if(joystickDer < 250){buf[1]=114;}
//  else if(joystickDer > 700){buf[1]=97;}
//  else if(joystickDer > 250 && joystickDer < 700){buf[1]=100;}
//  
//  driver.send(buf,buflen);
//  driver.waitPacketSent();
}
