#include <RH_ASK.h>
#include <SPI.h>
#include <Servo.h>

#define pinLed      13
#define pinEn       6
#define motorA      4
#define motorB      5
#define pinServo    9

RH_ASK driver;
Servo servo;

int velocidadMotor = 0;

void setup() {
  pinMode(pinLed, OUTPUT);
  pinMode(motorA, OUTPUT);
  pinMode(motorB, OUTPUT);
  servo.attach(pinServo);
  Serial.begin(9600);
  if(!driver.init())
    Serial.println("Inicialiación fallida");
}

void loop(){
  uint8_t buf[RH_ASK_MAX_MESSAGE_LEN];
  uint8_t buflen = sizeof(buf);
  if(driver.recv(buf, &buflen)){
    int i;
    if(buf[0] == 0){
      digitalWrite(motorA, LOW);
      digitalWrite(motorB, HIGH);
      digitalWrite(pinLed,LOW);
    }else{
      digitalWrite(motorA, HIGH);
      digitalWrite(motorB, LOW);
      digitalWrite(pinLed,HIGH);
    }

    if(buf[1] < 20){buf[1] = 0;}
    else if(buf[1] > 255){buf[1] = 255;}
    servo.write(buf[2]);
  }  
  analogWrite(pinEn,buf[1]);
}
