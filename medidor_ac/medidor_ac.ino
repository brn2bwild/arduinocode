#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>

#define pinLed  2

const char* ssid = "Tesalia";
const char* pass = "1123581321";
const char* mqtt_server = "192.168.5.51";

const float factor = 100;

const float multiplicador = 0.0625F;

WiFiClient espClient;
PubSubClient client(espClient);

//#define ADC_BITS        16
//#define ADC_CONTADOR    (1<<ADC_BITS)
Adafruit_ADS1115 ads;

void setup_wifi(){
  delay(10);
  Serial.println("");
  Serial.print("Conectando con: ");
  Serial.println(ssid);

  WiFi.begin(ssid, pass);

  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
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
    if(client.connect(clientID.c_str())){
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
  Serial.begin(115200);
  pinMode(pinLed, OUTPUT);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  ads.setGain(GAIN_TWO);
  ads.begin();
}

float obtenerCorriente(){
  float voltaje;
  float corriente;
  float suma = 0;
  long tiempo = millis();
  int contador = 0;
  
  while(millis() - tiempo < 1000){
    voltaje = ads.readADC_Differential_0_1() * multiplicador;
    corriente = voltaje * factor;
    corriente /= 1000.0;

    suma += sq(corriente);
    contador = contador + 1;
  }

  corriente = sqrt(suma / contador);
  return(corriente);
}

void loop(){
  reconectar();
 
  float corrienteRMS = obtenerCorriente();
  float potencia = 127 * corrienteRMS;
  Serial.print("Corriente: ");
  Serial.print(corrienteRMS);
  Serial.print(", Potencia: ");
  Serial.println(potencia);

  char mensajemqtt[10];
  //snprintf(mensajemqtt, 10, "%0.2F", corrienteRMS);
  dtostrf(corrienteRMS, 6, 4, mensajemqtt);
  client.publish("stat/medidor_edificiob/corriente", mensajemqtt);
  delay(3000);
}
