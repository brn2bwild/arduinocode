#include <ESP8266WiFi.h>
#include <PubSubClient.h>
// #include <Wire.h>
#include "ADS1115.h"

#define pinLed  2
//
//const char* ssid = "Tesalia";
//const char* pass = "1123581321";
//const char* mqtt_server = "192.168.40.6";

//const char* ssid = "_hoy-no";
//const char* pass = "a2h0o1r7a/";
//const char* mqtt_server = "192.168.1.68";

const char* ssid = "IoT-School";
const char* pass = "1123581321";
const char* mqtt_server = "192.168.43.163";

const float factor = 100;
const float multiplicador = 0.0625F;

WiFiClient espClient;
PubSubClient client(espClient);

ADS1115 ads(ADS1115_DEFAULT_ADDRESS);

void setup_wifi(){
  delay(10);
  // Serial.println("");
  // Serial.print("Conectando con: ");
  // Serial.println(ssid);

  WiFi.begin(ssid, pass);

  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    // Serial.print(".");
  }

  // Serial.println("");
  // Serial.println("WiFi connected");
  // Serial.println("IP address: ");
  // Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length){
  Serial.print("Mensaje recibido [");
  Serial.print(topic);
  Serial.print("] ");
  for(int i = 0; i < length; i++){
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconectar(){
  while(!client.connected()){
    Serial.println("Intentando conectar con el servidor...");
    String clientID = "ESP8266Client-";
    clientID += String(random(0xffff), HEX);
    if(client.connect(clientID.c_str(),"stat/medidor_edificiob/LWT",0,false,"Offline")){
      Serial.println("Conectado");
      client.publish("stat/medidor_edificiob/LWT", "Online");
      client.subscribe("stat/medidor_edificiob/STATUS");
    }else{
      Serial.print("Fallo, rc= ");
      Serial.print(client.state());
      Serial.println("Intentando de nuevo en 5s");
      delay(5000);
    }
  }
}

void setup(){
    Wire.begin();
    Serial.begin(115200);
    pinMode(pinLed, OUTPUT);

    setup_wifi();
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
    
    Serial.println("Probando conexión con el ADS");
    Serial.println(ads.testConnection() ? "Conexión exitosa" : "Conexión fallida");

    ads.initialize();

    ads.setMode(ADS1115_MODE_SINGLESHOT);
    ads.setRate(ADS1115_RATE_860);
    ads.setGain(ADS1115_PGA_2P048);
}

float obtenerCorriente(){
  float corrienteInstantanea, sumatoria = 0;
  uint32_t tiempoInicio = millis();
  int contador = 0;

  ads.setMultiplexer(ADS1115_MUX_P0_N1);
  
  while(millis() - tiempoInicio < 1000){
    ads.triggerConversion();
    while(!ads.isConversionReady());
    corrienteInstantanea = (ads.getMilliVolts(false)/1000.0) * factor;
    sumatoria += corrienteInstantanea*corrienteInstantanea;
    contador++;
  }

  float corrienteRMS = sqrt(sumatoria / contador);
  return corrienteRMS;
}

float obtenerVoltaje(){
  // float valorADC, voltINST, sumatoria = 0, voltaje;
  // unsigned long tiempoINICIO = millis();
  // int n = 0;

  // while((millis() - tiempoINICIO) < 1000){
  //   valorADC = ads.readADC_SingleEnded(2);
  //   voltINST = map(valorADC, adc_min, adc_max, voltaje_multi_n, voltaje_multi_p);
  //   sumatoria = sumatoria + sq(voltINST);
  //   n = n + 1;
  //   delay(1);
  // }

  // voltaje = sqrt(sumatoria/n);
  // return(voltaje);
  ads.setMultiplexer(ADS1115_MUX_P2_NG);
  uint32_t tiempoMedicion = 2000;
  uint32_t tiempoInicio = millis();

  float sumatoria = 0, voltajeInstantaneo, voltajeADC;
  int contadorMedidas = 0;

  while(millis() - tiempoInicio < tiempoMedicion){
    ads.triggerConversion();
    while(!ads.isConversionReady());
    voltajeInstantaneo = (ads.getMilliVolts(false)/1000.0) - 1.633;
    sumatoria += voltajeInstantaneo*voltajeInstantaneo;
    contadorMedidas++;
  }

  float voltajeRMS = (sqrt(sumatoria/contadorMedidas) * 190.94);
  return voltajeRMS;
}

void loop(){
  reconectar();
 
//   float voltajeRMS = random(126.0,128.0);
  float voltajeRMS = obtenerVoltaje();
  float corrienteRMS = obtenerCorriente();
  float potenciaAparente = voltajeRMS * corrienteRMS;
  Serial.print("Voltaje: ");
  Serial.print(voltajeRMS);
  Serial.print("V, Corriente: ");
  Serial.print(corrienteRMS);
  Serial.print("A, Potencia: ");
  Serial.print(potenciaAparente);
  Serial.println("VA");

  char mensajemqtt[10];

  dtostrf(voltajeRMS, 6, 4, mensajemqtt);
  client.publish("stat/medidor_edificiob/voltaje", mensajemqtt);
  memset(mensajemqtt, 0, sizeof(mensajemqtt));

  dtostrf(corrienteRMS, 6, 4, mensajemqtt);
  client.publish("stat/medidor_edificiob/corriente", mensajemqtt);
  memset(mensajemqtt, 0, sizeof(mensajemqtt));

  dtostrf(potenciaAparente, 6, 4, mensajemqtt);
  client.publish("stat/medidor_edificiob/potencia", mensajemqtt);
  delay(2000);

  // float voltaje_rms = obtenerVoltaje();
  // Serial.print("Vrms: ");
  // Serial.print(voltaje_rms, 3);
  // Serial.println("V");
  // delay(1000);

  // float corriente = ((ads.readADC_Differential_0_1() * multiplicador) * factor) / 1000.0;
  // float corriente = obtenerCorriente();
  // Serial.println(corriente);
  // delay(100);

}
