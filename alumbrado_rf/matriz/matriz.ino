//#include <CayenneMQTTESP8266.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <RH_ASK.h>
#include <SPI.h>


#define pinBoton    4
#define pinLed      2
#define pinRele     0
// #define pinLed    13 //sonoff
// #define pinBoton  0  //sonoff
// #define pinRele   12 //sonoff

#define estadoON    "ON"
#define estadoOFF   "OFF"

#define intervaloVerificacionServidor 10000
#define intParpadeoNoOk 200

unsigned long tiempoAnteriorservidor = 0;

const char *ssid_wifi = "Tesalia";
const char *pass_wifi = "hhoyy///";

bool estado_sistema = false;
bool mensaje_rf_recibido = false;

RH_ASK driver(2000, 16, 5);
ESP8266WebServer server;
WiFiClient espClient;
PubSubClient mqttClient(espClient);

void reconectarServidor(){
  if(!mqttClient.connected()){
    Serial.println("Intentado conectar con el servidor");
    if(mqttClient.connect("nodoRF01", "sistema/estado/nodorf01", 0, true, "OFF")){
      Serial.println("Dispositivo conectado con el servidor");
      mqttClient.publish("sistema/estado/nodorf01","ON",true);
      mqttClient.subscribe("sistema/comando/nodorf01");
    }else{
      Serial.println("Dispositivo no se pudo conectar con el servidor");
      Serial.println("Intentando reconectar en 10 segundos");
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length){
  String strTopic = (char*)topic;
  char* msj;
  Serial.print("Mensaje recibido [");Serial.print(topic);Serial.print("] ");
  Serial.println((char*)payload);
    
  // if(strcmp(cadenaPayload, estadoON) == 0){
  if(strTopic == "sistema/comando/nodorf01"){
    if(payload[0] == '1'){
      digitalWrite(pinLed, LOW);
      msj = "M,1";
      mqttClient.publish("sistema/estado/nodorf01", "ON",true);
    // }else if(strcmp(cadenaPayload, estadoOFF) == 0){
    }else if(payload[0] == '0'){
      digitalWrite(pinLed, HIGH);
      msj = "M,0";
      mqttClient.publish("sistema/estado/nodorf01", "OFF",true);
    }
  }
  driver.send((uint8_t *)msj, strlen(msj));
  driver.waitPacketSent();
  Serial.println((char *)msj);
}

void setup() {
  
  pinMode(pinRele, OUTPUT);
  pinMode(pinLed, OUTPUT);
  pinMode(pinBoton, INPUT);
  digitalWrite(pinRele, HIGH);

  Serial.begin(115200);

  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid_wifi, pass_wifi);

  if(!driver.init())
    Serial.println("Inicialización fallida");

  while(WiFi.status()!=WL_CONNECTED){
    Serial.print(".");
    delay(500);
  }

  Serial.println("\nIP: "); Serial.println(WiFi.localIP());
  server.on("/",[](){server.send(200,"text/plain","Dispositivo funcionando");});
  server.on("/toggle",cambiarEstado);
  server.begin();

  mqttClient.setServer("192.168.40.5",1883);
  mqttClient.setCallback(callback);
}

void loop() {
  uint8_t buffer_entrada[11];
  uint8_t buffer_entrada_len = sizeof(buffer_entrada);
  unsigned long ahora = millis();
  
  if(!digitalRead(pinBoton)){
    delay(250);
    cambiarEstado();
  }
  
  if(!mqttClient.connected() && (unsigned long)ahora - tiempoAnteriorservidor > intervaloVerificacionServidor){
    tiempoAnteriorservidor = millis();
    reconectarServidor();
  }
  mqttClient.loop();
  server.handleClient();

  if(driver.recv(buffer_entrada, &buffer_entrada_len)){
    int i;
    mensaje_rf_recibido = true;
  }

  if(mensaje_rf_recibido){
    Serial.print("Mensaje de estado recibido:");
    Serial.println((char*)buffer_entrada);
    // if(mqttClient.connected()){
    //     mqttClient.publish("sistema/nodorf01/001/errorA",(char)buffer_entrada[6],true);
    //     mqttClient.publish("sistema/nodorf01/001/errorV",(char)buffer_entrada[8],true);
    // }
    mensaje_rf_recibido = false;
  }
}

void cambiarEstado(){
  const char *msj;
  if(estado_sistema){
    msj = "M,1";
    digitalWrite(pinLed, LOW);
    server.send(200,"text/plain","Estado encendido");
    if(mqttClient.connected())
      mqttClient.publish("sistema/estado/nodorf01","ON",true);
  }else{
    msj = "M,0";
    digitalWrite(pinLed, HIGH);
    server.send(200,"text/plain","Estado apagado");
    if(mqttClient.connected())
      mqttClient.publish("sistema/estado/nodorf01","OFF",true);
  }
  estado_sistema = !estado_sistema;
  driver.send((uint8_t *)msj, strlen(msj));
  driver.waitPacketSent();
  Serial.println((char *)msj);

  while(!digitalRead(pinBoton));
}