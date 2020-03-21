#include <EEPROM.h>

#define pinMonedero           2
#define pinLed                13
#define pinBotonReset         5
#define pinBotonProducto1     6
#define pinBotonProducto2     7
#define pinBotonAgua          8
#define pinBombaDispensador   12
#define pinBombaDeposito      11
#define pinMotorProducto      10

#define eeprom_sAgua  0
#define eeprom_sPro1  1

unsigned long tiempoAnterior = 0;
//volatile int cantidadMonedas = 0;
volatile bool credito = false;
bool productoServido = true;
volatile unsigned long tiempoPulsoAnterior = 0;
unsigned char contadorSegProducto1 = 0;
unsigned char contadorSegAgua = 0;

void setup() {
  pinMode(pinBotonProducto1, INPUT_PULLUP);
  pinMode(pinBotonProducto2, INPUT_PULLUP);
  pinMode(pinBotonAgua, INPUT_PULLUP);
  pinMode(pinMonedero, INPUT_PULLUP);
  pinMode(pinBotonReset, INPUT_PULLUP);
  pinMode(pinLed, OUTPUT);
  pinMode(pinBombaDispensador, OUTPUT);
  pinMode(pinBombaDeposito, OUTPUT);
  pinMode(pinMotorProducto, OUTPUT);
  digitalWrite(pinBombaDispensador, HIGH);
  digitalWrite(pinBombaDeposito, HIGH);
  digitalWrite(pinMotorProducto, HIGH);

  Serial.begin(9600);

  contadorSegAgua = EEPROM.read(eeprom_sAgua);
  contadorSegProducto1 = EEPROM.read(eeprom_sPro1);

  if(!digitalRead(pinBotonReset)){
    Serial.println("modo configuracion");

    int opcion = -1;
    while(1){
      if(!digitalRead(pinBotonAgua)){
        if(opcion++ >= 2) opcion = 0;
        delay(200);
        mostrarMenu(opcion);
      }
    }
  }

  attachInterrupt(digitalPinToInterrupt(pinMonedero), contadorMonedas, FALLING);

  credito = false;
}

void mostrarMenu(int opcion){
  switch (opcion) {
      case 0:
        Serial.println("editanto segundos de cantidad de agua");
        while(digitalRead(pinBotonAgua)){
          if(!digitalRead(pinBotonProducto1)){
            contadorSegAgua++;
            delay(200);
            EEPROM.write(eeprom_sAgua, contadorSegAgua);
            Serial.println(contadorSegAgua);
          }else if(!digitalRead(pinBotonProducto2)){
            contadorSegAgua--;
            delay(200);
            EEPROM.write(eeprom_sAgua, contadorSegAgua);
            Serial.println(contadorSegAgua);
          }
        }
        break;
      case 1:
        Serial.println("editanto segundos de cantidad de producto");
        while(digitalRead(pinBotonAgua)){
          if(!digitalRead(pinBotonProducto1)){
            contadorSegProducto1++;
            delay(300);
            EEPROM.write(eeprom_sPro1, contadorSegProducto1);
            Serial.println(contadorSegProducto1);
          }else if(!digitalRead(pinBotonProducto2)){
            contadorSegProducto1--;
            delay(300);
            EEPROM.write(eeprom_sPro1, contadorSegProducto1);
            Serial.println(contadorSegProducto1);
          }
        }
        break;
  }
}

void contadorMonedas(){
  if(millis() - tiempoPulsoAnterior > 60){
    //cantidadMonedas++;
    credito = true;
    productoServido = false;
    tiempoPulsoAnterior = millis();
  }
}
void servicioProducto(){
  Serial.println("servicio producto");
  
  digitalWrite(pinMotorProducto, LOW);
  delay(int(contadorSegProducto1*1000));
  digitalWrite(pinMotorProducto, HIGH); 
}

void servicioAgua(){
  Serial.println("servicio agua");

  digitalWrite(pinBombaDispensador, LOW);
  digitalWrite(pinBombaDeposito, LOW);
  delay(int(contadorSegAgua*1000));
  digitalWrite(pinBombaDispensador, HIGH);
  digitalWrite(pinBombaDeposito, HIGH);

  if(credito) credito = false;
}

void loop() {
  if(!digitalRead(pinBotonProducto1) && credito){
    delay(250);
    servicioProducto();
    if(!digitalRead(pinBotonAgua)) servicioAgua();
  } else if(!digitalRead(pinBotonAgua) && credito){
    delay(250);
    servicioAgua();
  } else if((!digitalRead(pinBotonAgua) || !digitalRead(pinBotonProducto1)) && !credito) Serial.println("Introduzca monedas");

  unsigned long ahora = millis();

  if(ahora - tiempoAnterior > 1000){
    tiempoAnterior = ahora;
    digitalWrite(pinLed, !digitalRead(pinLed));
    Serial.print(credito);
    Serial.print(",");
    Serial.print(contadorSegAgua);
    Serial.print(",");
    Serial.println(contadorSegProducto1);
  }
}
