const int pinLed = 13;
const int pinBoton1led = 5;
const int pinBoton2led = 6;
const int pinBoton3led = 7;
const int pinBoton1push = 8;
const int pinBoton2push = 9;
const int pinBoton3push = 10;
const int pinValvula = 11;
const int pinMonedero = 2;
const int pinSensorflujo = 3;

volatile float dineroTotal = 0.00;
bool banderaMoneda = false;

float flujoInmediato = 0.0, calibracion = 4.5;
volatile unsigned int contadorFlujo = 0;
unsigned long tiempoAnteriorsensor = 0;
//int mililitrosAservir = 0, mililitrosServidos = 0;



void setup() {
  Serial.begin(9600);
  pinMode(pinLed, OUTPUT);
  pinMode(pinBoton1led, OUTPUT);
  pinMode(pinBoton2led, OUTPUT);
  pinMode(pinBoton3led, OUTPUT);
  pinMode(pinValvula, OUTPUT);
  pinMode(pinBoton1push, INPUT);
  pinMode(pinBoton2push, INPUT);
  pinMode(pinBoton3push, INPUT);
  pinMode(pinMonedero, INPUT);
  pinMode(pinSensorflujo, INPUT);
  attachInterrupt(0, interrupcionMonedero, RISING);
  attachInterrupt(1, interrupcionSensor, RISING);
}
void interrupcionSensor(){
  contadorFlujo++;
}

void interrupcionMonedero(){
  dineroTotal = dineroTotal + 1.0;
  banderaMoneda = true;  
}

void dispensarProducto(unsigned int mlaServir){
  int milimetrosServidos = 0;
  digitalWrite(pinValvula, HIGH);
  while((mlaServir - mililitrosServidos) > 0){
    if((unsigned long)(millis() - tiempoAnteriorsensor) > 1000){
      detachInterrupt(1);

      flujoInmediato = ((1000.0 / (millis() - tiempoAnteriorsensor)) * contadorFlujo) / calibracion;

      tiempoAnteriorsensor = millis();
      mililitrosServidos = mililitrosServidos + ((flujoInmediato / 60) * 1000);
      contadorFlujo = 0;
      attachInterrupt(1, interrupcionSensor, RISING);
    }
  }
  digitalWrite(pinValvula, LOW);
}
void loop() {
  if(banderaMoneda){
    banderaMoneda = false;

    Serial.print("Creditos: $");
    Serial.println(dineroTotal);
  }

  if(dineroTotal > 1.0){
    digitalWrite(pinBoton1led, HIGH);
  }else{
    digitalWrite(pinBoton1led, LOW);
  }
  if(dineroTotal > 4.0){
    digitalWrite(pinBoton2led, HIGH);
  }else{
    digitalWrite(pinBoton2led, LOW);
  }
  if(dineroTotal > 7.0){
    digitalWrite(pinBoton3led, HIGH);
  }else{
    digitalWrite(pinBoton3led, LOW);
  }

  if(!digitalRead(pinBoton1push)){
    if(dineroTotal - 1.0 >= 0.0){
      dineroTotal = dineroTotal - 1.0;
      Serial.println("Servir un litro");
      digitalWrite(pinBoton2led, LOW);
      digitalWrite(pinBoton3led, LOW);
      dispensarProducto(1000);
      Serial.println("Medio garrafón servido");
    }else{
      Serial.println("Crédito insuficiente");
    }
  }
  if(!digitalRead(pinBoton2push)){
    if(dineroTotal - 4.0 >= 0.0){
      dineroTotal = dineroTotal - 4.0;
      Serial.println("Servir medio garrafón");
      digitalWrite(pinBoton1led, LOW);
      digitalWrite(pinBoton3led, LOW);
      dispensarProducto(10000);
      Serial.println("Medio garrafón servido");
    }else{
      Serial.println("Crédito insuficiente");
    }
  }
  if(!digitalRead(pinBoton3push)){
    if(dineroTotal - 7.0 >= 0.0){
      dineroTotal = dineroTotal - 7.0;
      Serial.println("Servir un garrafón");
      digitalWrite(pinBoton1led, LOW);
      digitalWrite(pinBoton2led, LOW);
      dispensarProducto(20000);
      Serial.println("Garrafón servido");
    }else{
      Serial.println("Crédito insuficiente");
    }
  }
}
