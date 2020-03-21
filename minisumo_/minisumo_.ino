// DECLARACION DE VARIABLES PARA PINES
const int pinecho = 11;
const int pintrigger = 12;
const int pinled = 13;
const int pinsen1 = 9;
const int pinsen2 = 10;
const int motor1a = 2;
const int motor1b = 3;
const int motor2a = 4;
const int motor2b = 5;
const int enable = 6;
 
// VARIABLES PARA CALCULOS
unsigned int tiempo, distancia;
 
void setup() {
  // PREPARAR LA COMUNICACION SERIAL
  Serial.begin(9600);
  // CONFIGURAR PINES DE ENTRADA Y SALIDA
  pinMode(pinecho, INPUT);
  pinMode(pintrigger, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(pinsen1, INPUT);
  pinMode(pinsen2, INPUT);
  pinMode(motor1a, OUTPUT);
  pinMode(motor1b, OUTPUT);
  pinMode(motor2a, OUTPUT);
  pinMode(motor2b, OUTPUT);
  pinMode(enable, OUTPUT);


  delay(4000);
  digitalWrite(enable,HIGH);
  digitalWrite(motor1a,HIGH);
  digitalWrite(motor1b,LOW);
  digitalWrite(motor2a,HIGH);
  digitalWrite(motor2b,LOW);
  
}
 
void loop() {
  // ENVIAR PULSO DE DISPARO EN EL PIN "TRIGGER"
  digitalWrite(pintrigger, LOW);
  delayMicroseconds(2);
  digitalWrite(pintrigger, HIGH);
  // EL PULSO DURA AL MENOS 10 uS EN ESTADO ALTO
  delayMicroseconds(10);
  digitalWrite(pintrigger, LOW);
 
  // MEDIR EL TIEMPO EN ESTADO ALTO DEL PIN "ECHO" EL PULSO ES PROPORCIONAL A LA DISTANCIA MEDIDA
  tiempo = pulseIn(pinecho, HIGH);
 
  // LA VELOCIDAD DEL SONIDO ES DE 340 M/S O 29 MICROSEGUNDOS POR CENTIMETRO
  // DIVIDIMOS EL TIEMPO DEL PULSO ENTRE 58, TIEMPO QUE TARDA RECORRER IDA Y VUELTA UN CENTIMETRO LA ONDA SONORA
  distancia = tiempo / 58;
 
  // ENCENDER EL LED CUANDO SE CUMPLA CON CIERTA DISTANCIA
  if (distancia <= 25) {
    digitalWrite(13, HIGH);
    digitalWrite(enable,HIGH);
    digitalWrite(motor1a,HIGH);
    digitalWrite(motor1b,LOW);
    digitalWrite(motor2a,LOW);
    digitalWrite(motor2b,HIGH);
  } else {
    digitalWrite(13, LOW);
    digitalWrite(enable,HIGH);
    digitalWrite(motor1a,HIGH);
    digitalWrite(motor1b,LOW);
    digitalWrite(motor2a,HIGH);
    digitalWrite(motor2b,LOW);
    if(digitalRead(pinsen1) == LOW || digitalRead(pinsen2) == LOW){
      digitalWrite(13,HIGH);
      digitalWrite(enable,HIGH);
      digitalWrite(motor1a,LOW);
      digitalWrite(motor1b,HIGH);
      digitalWrite(motor2a,HIGH);
      digitalWrite(motor2b,LOW);
    }else{
      digitalWrite(13,LOW);
    }
  }
  
  
  
}
