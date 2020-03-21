#include <RH_ASK.h>
#include <SPI.h>
// #define pinVBateria       A0
// #define pinVPaneles       A1
// #define pinCPaneles       A2
// #define pinCCarga         A3
// #define pinSTemperatura   A4
const int volBateria = 0;
const int volPaneles = 1;
const int corPaneles = 2;
const int corCarga   = 3;
const int temPaneles = 4;
const int pinLed     = 13;

RH_ASK driver;

void setup() {
  for(int i = 0; i < 5; i++) pinMode(i, INPUT);
  pinMode(pinLed, OUTPUT);
  Serial.begin(9600);  

  if(!driver.init()) Serial.println("Error al inicializar el módulo RF");
}

void loop() {
  enviarDatos(volPaneles);
  delay(1000);
  enviarDatos(volBateria);
  delay(1000);
  enviarDatos(corPaneles);
  delay(1000);
  enviarDatos(corCarga);
  delay(1000);
  enviarDatos(temPaneles);
  delay(1000);
}

void enviarDatos(int dato){
  // Serial.println(dato);
  char* bufferDatos = "";  
  char cadena[12];

  dtostrf(medir(dato), 6, 2, bufferDatos);

  if(dato == 0){
    snprintf(cadena, 12, "vB;%s;", bufferDatos);
  }else if(dato == 1){
    snprintf(cadena, 12, "vP;%s;", bufferDatos);
  }else if(dato == 2){
    snprintf(cadena, 12, "cP;%s;", bufferDatos);
  }else if(dato == 3){
    snprintf(cadena, 12, "cC;%s;", bufferDatos);   
  }else if(dato == 4){
    snprintf(cadena, 12, "tP;%s;", bufferDatos);
  }

  Serial.println(cadena);
    
  driver.send((uint8_t *)cadena, strlen(cadena));
  driver.waitPacketSent();
}

// float temperatura(){
//   analogReference(INTERNAL);
//   int valor[5];
//   int sumatoria = 0;

//   for(int i = 0; i < 5; i++){
//     valor[i] = analogRead(A4);
//     delay(5);
//   }

//   for(int j = 1; j < 4; j++) sumatoria += valor[j];

  // Serial.println(sumatoria/3);
//   return (-0.2576 * (sumatoria / 3) + 167.12);
// }

float medir(int canal){
  int valor[5];
  int sumatoria = 0;
  int aux;
 
  if(canal == corPaneles || canal == corCarga){
    analogReference(DEFAULT);
  }else{
    analogReference(INTERNAL);
  }

  for(int i = 0; i < 5; i++){
    valor[i] = analogRead(canal);
    delay(5);
  }

  for(int j = 0; j < 5; j++){
    if(valor[j+1] > valor[j]){
      aux = valor[j];
      valor[j] = valor[j+1];
      valor[j+1] = aux; 
    }
  }

  for(int k = 1; k < 4; k++) sumatoria += valor[k];

  // Serial.println(sumatoria/3);

  if(canal == volBateria){
    return ((0.0299 * (sumatoria / 3)) - 0.0601);
  }else if(canal == volPaneles){
    return ((0.0296 * (sumatoria / 3)) - 0.0869);
  }else if(canal == corPaneles || canal == corCarga){
  //   return (((sumatoria / 3) * (5.0 / 1023.0)) - 2.5) / 0.066;
    return ((0.0738 * (sumatoria / 3)) - 37.563); // cálculo hecho mediante regresión
  }else{
    return ((-0.2679 * (sumatoria / 3)) + 172.54);
  }

  // return (sumatoria/3);
}

// float corriente(int canal){
//   analogReference(DEFAULT);
//   int corriente[5];
//   int sumatoria = 0;
 
//   for(int i = 0; i < 5; i++){
//     corriente[i] = analogRead(canal);
//     delay(5);
//   }

//   for(int j = 1; j < 4; j++) sumatoria += corriente[j];

//   return (((sumatoria / 3) * (5.0 / 1023.0)) - 2.5) / 0.066;
// }

// float voltaje(int canal){
//   analogReference(INTERNAL);
//   int voltaje[5];
//   int sumatoria = 0;

//   for(int i = 0; i < 5; i++){
//     voltaje[i] = analogRead(canal);
//     delay(5);
//   }

//   for(int j = 1; j < 4; j++) sumatoria += voltaje[j];

//   if(canal == 0){
//     return (0.0244 * (sumatoria / 3) + 2.1318);
//   }else{
//     return (0.0312 * (sumatoria / 3) - 0.4354);
//     //return (0.0305 * (sumatoria / (muestras - 4)) - 0.2061);
//   }
// }
