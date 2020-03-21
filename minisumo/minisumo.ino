#define ledPin    13
#define boton1    8
#define sensorFronIzq  A1 //A1
#define sensorFronDer  A0 //A0
#define sensorLinIzq  11
#define sensorLinDer  12
#define motorA1   7   //7
#define motorAen  6
#define motorA2   5   //5
#define motorB1   4
#define motorBEn  3
#define motorB2   2

void setup() {
  //Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
  pinMode(boton1, INPUT);
  pinMode(sensorFronIzq, INPUT);
  pinMode(sensorFronDer, INPUT);
  pinMode(sensorLinIzq, INPUT);
  pinMode(sensorLinDer, INPUT);
  pinMode(motorA1, OUTPUT);
  pinMode(motorAen, OUTPUT);
  pinMode(motorA2, OUTPUT);
  pinMode(motorB1, OUTPUT);
  pinMode(motorBEn, OUTPUT);
  pinMode(motorB2, OUTPUT);

  digitalWrite(motorAen, HIGH);
  digitalWrite(motorBEn, HIGH);

  motores(0,0);
  
  while(digitalRead(boton1)){
    delay(100);
    digitalWrite(ledPin,HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);   
  }

  pausa();
  arranque_linea();
  
}

void arranque_linea(){
  motores(70, 80);
  
  while(digitalRead(sensorLinIzq) && digitalRead(sensorLinDer)){
    if(!digitalRead(sensorFronIzq) && !digitalRead(sensorFronDer)){
      break;
    }
  }
  if(!digitalRead(sensorLinIzq) || !digitalRead(sensorLinDer)){
    freno(true, true, 255); //se detienen los motores
    motores(-240, -255); //se retrocede
    delay(480);
    freno(true, true, 255);
    delay(30);
    motores(230, -230); //se gira a la derecha
    delay(150);
    freno(true, true, 255);
  }
}

void arranque(){
  motores(-255, 255);
  
  while(digitalRead(sensorFronIzq) && digitalRead(sensorFronDer));
  
  freno(true, true, 255); //se detienen los motores
}

void pausa(){
  for(int i=0; i<5; i++){
    delay(900);
    digitalWrite(ledPin,HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);   
  }
}

void freno(bool izquierda, bool derecha, int valor){
  if(izquierda){
    digitalWrite(motorB1, HIGH);
    digitalWrite(motorB2, HIGH);
    analogWrite(motorBEn, valor);
  }
  if(derecha){
    digitalWrite(motorA1, HIGH);
    digitalWrite(motorA2, HIGH);
    analogWrite(motorAen, valor);
  }
}
void motor_izquierdo(int velocidad){
  if(velocidad > 255) velocidad = 255;
  else if(velocidad < -255) velocidad = -255;
  
  if( velocidad >= 0){
    digitalWrite(motorB1, HIGH);
    digitalWrite(motorB2, LOW);
  }else{
    digitalWrite(motorB1, LOW);
    digitalWrite(motorB2, HIGH);
    velocidad *= -1;
  }
  analogWrite(motorBEn, velocidad);
}

void motor_derecho(int velocidad){
  if(velocidad > 255) velocidad = 255;
  else if(velocidad < -255) velocidad = -255;

  if(velocidad >= 0){
    digitalWrite(motorA1, LOW);
    digitalWrite(motorA2, HIGH);
  }else{
    digitalWrite(motorA1, HIGH);
    digitalWrite(motorA2, LOW);
    velocidad *= -1;    
  }
  analogWrite(motorAen, velocidad);
}

void motores(int vIzquierdo, int vDerecho){
  motor_izquierdo(vIzquierdo);
  motor_derecho(vDerecho);
}

//void derecha(){
//  digitalWrite(motorA1, HIGH);
//  digitalWrite(motorA2, LOW);
//  digitalWrite(motorB1, HIGH);
//  digitalWrite(motorB2, LOW);
//}
//
//void izquierda(){
//  digitalWrite(motorA1, LOW);
//  digitalWrite(motorA2, HIGH);
//  digitalWrite(motorB1, LOW);
//  digitalWrite(motorB2, HIGH);
//}
//
//void atras(){
//  digitalWrite(motorA1, HIGH);
//  digitalWrite(motorA2, LOW);
//  digitalWrite(motorB1, LOW);
//  digitalWrite(motorB2, HIGH);
//}
//
//void adelante(){
//  digitalWrite(motorA1, LOW);
//  digitalWrite(motorA2, HIGH);
//  digitalWrite(motorB1, HIGH);
//  digitalWrite(motorB2, LOW);
//}
//
//void detener(){
//  digitalWrite(motorA1, LOW);
//  digitalWrite(motorA2, LOW);
//  digitalWrite(motorB1, LOW);
//  digitalWrite(motorB2, LOW);
//}

void sensoresLinea(){
  
}

void loop() {
  if(!digitalRead(sensorLinIzq) || !digitalRead(sensorLinDer)){
    digitalWrite(ledPin, HIGH);
    freno(true, true, 255);
    delay(5);
    motores(-255, -255);
    delay(100);
    motores(-100, 100);
  }else{
    if(!digitalRead(sensorFronIzq) && !digitalRead(sensorFronDer)){
      digitalWrite(ledPin, HIGH);
      motores(255, 255);
    }else if(!digitalRead(sensorFronIzq) && digitalRead(sensorFronDer)){
      digitalWrite(ledPin, HIGH);
      motores(-150, 150);
    }else if(digitalRead(sensorFronIzq) && !digitalRead(sensorFronDer)){
      digitalWrite(ledPin, HIGH);
      motores(150, -150);
    }else{
      digitalWrite(ledPin, LOW);
      motores(-150, 150);
    }
  }
}
