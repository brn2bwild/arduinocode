#include <QTRSensors.h>

#define ledPin  13
#define pwmD    3
#define motorDA 4 
#define motorDB 5
#define motorIA 7
#define motorIB 8
#define pwmI    9
#define stnBy   6
#define boton   10

// int vbase = 80;
// int p, u, d;
// float i = 0;
// float p_anterior = 0;

QTRSensors qtr;

const uint8_t cantidad_sensores = 8;
uint16_t SenIR[cantidad_sensores];

void setup() {
  Serial.begin(9600);
  qtr.setTypeAnalog();
  qtr.setSensorPins((const uint8_t[]){A7, A6, A5, A4, A3, A2, A1, A0}, cantidad_sensores);
  
  delay(500);

  pinMode(boton, INPUT);
  pinMode(motorIA, OUTPUT);
  pinMode(motorIB, OUTPUT);
  pinMode(pwmI, OUTPUT);
  pinMode(motorDA, OUTPUT);
  pinMode(motorDB, OUTPUT);
  pinMode(pwmD, OUTPUT);

  motor(0, 0);
  digitalWrite(stnBy, HIGH);

  while(digitalRead(boton)){
    digitalWrite(ledPin, !digitalRead(ledPin));
    delay(500);
  }

  for(uint16_t i = 0; i < 150; i++){
    digitalWrite(ledPin, HIGH); delay(20);
    qtr.calibrate();
    digitalWrite(ledPin, LOW);  delay(20);
  }

  digitalWrite(ledPin, HIGH);

  while(digitalRead(boton));

  delay(1000);
  
}

void motorIzquierdo(int valor)
{  
  if ( valor >= 0 )
  {
    digitalWrite(motorIA,LOW);
    digitalWrite(motorIB,HIGH);
  }
  else
  {
    digitalWrite(motorIA,HIGH);
    digitalWrite(motorIB,LOW);
    valor *= -1;
  }    
  analogWrite(pwmI,valor);
}

void motorDerecho(int valor)
{  
  if ( valor >= 0 )
  {
    digitalWrite(motorDA,LOW);
    digitalWrite(motorDB,HIGH);
  }
  else
  {
    digitalWrite(motorDA,HIGH);
    digitalWrite(motorDB,LOW);
    valor *= -1;
  }    
  analogWrite(pwmD,valor);
}

void freno(boolean izquierdo, boolean derecho, int valor)
{
  if ( izquierdo )
  {
    digitalWrite(motorIA,HIGH);
    digitalWrite(motorIB,HIGH);
    analogWrite (pwmI, valor);
  }
  if ( derecho )
  {
    digitalWrite(motorDA,HIGH);
    digitalWrite(motorDB,HIGH);
    analogWrite (pwmD, valor);
  }
}

void motor(int izquierda, int derecha){
  motorIzquierdo(izquierda);
  motorDerecho(derecha);
}

//con ésta velocidad los valores son p = 0.025, i = 0.0, d = 0.385
//int vMax = 255;

//---------- estas constantes son por si la pista tiene muchas curvas
const float kp = 0.047; //0.047
const float ki = 0.0;   
const float kd = 0.645;//0.645

// velocidad del sigue líneas
int vMax = 180;

int proporcional = 0;
int integral = 0;
int derivativo = 0;

long error;
int ultimo_proporcional;
int objetivo = 3500;

int vel_motor_izquierdo;
int vel_motor_derecho;

void loop(){
  // motor(vMax, vMax); 

  unsigned int position = qtr.readLineBlack(SenIR);

  // qtr.read(SenIR);

  // for(int i = 0; i < 8; i++){
  //   Serial.print(SenIR[i]);
  //   Serial.print('\t');
  // }
  // Serial.println(position);

  proporcional = ((int)position) - 3500;

  Serial.println(position);

  if(proporcional <= -objetivo){
    motorIzquierdo(0);
    freno(true, false, 255);
  }else if(proporcional >= objetivo){
    motorDerecho(0);
    freno(false, true, 255);
  }

  integral += ultimo_proporcional;

  //if(integral > 1000) integral = 1000;
  //if(integral < -1000) integral = -1000;
  
  derivativo = proporcional - ultimo_proporcional;

  ultimo_proporcional = proporcional;

  error = (proporcional * kp) + (integral * ki) + (derivativo * kd);

  //------ estas líneas son por si la pista tiene muchas curvas difíciles----------//
  vel_motor_izquierdo = vMax + error;
  vel_motor_derecho = vMax - error;

  if(vel_motor_izquierdo < -255) vel_motor_izquierdo = -255;
  else if(vel_motor_izquierdo > 255) vel_motor_izquierdo = 255;

  if(vel_motor_derecho < -255) vel_motor_derecho = -255;
  else if(vel_motor_derecho > 255) vel_motor_derecho = 255;

  motor(vel_motor_izquierdo, vel_motor_derecho);
  
}
