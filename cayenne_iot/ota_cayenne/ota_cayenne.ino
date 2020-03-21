/* con éste sketch los módulos de prueba podrán ser reprogramados através de la red WiFi*/
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <CayenneMQTTESP8266.h>

#ifndef STASSID
#define STASSID "_hoy-no"
#define STAPSK  "a2h0o1r7a/"
#endif

#define canal 0

const char* ssid = STASSID;
const char* password = STAPSK;

const char cayUser[] = "abe72a70-31fd-11e7-aa05-db50d12f1f3d";
const char cayPass[] = "e91b50872db8081c5bb705d2e9c7256124e4ca5d";
const char cayID[] = "1d64a9d0-84ee-11e9-94e9-493d67fd755e";

const int pinLed = 13;
const int pinBoton = 0;
const int pinRele = 12;

const unsigned long  intervaloLed = 2000;

bool ota_flag = true;
unsigned long tiempo_anterior = 0;

ESP8266WebServer Server;

void setup() {
  pinMode(pinLed, OUTPUT);
  pinMode(pinRele, OUTPUT);
  pinMode(pinBoton, INPUT);

  digitalWrite(pinRele, HIGH);

  Serial.begin(115200);
  Serial.println("Iniciando WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("CConexion WiFi fallida, reiniciando...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("Corredor");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Server.on("/restart",[](){
    Server.send(200,"text/plain", "Reiniciando...");
    delay(1000);
    ESP.restart();
  });

  Server.on("/",[](){
    Server.send(200,"text/plain", "Ok");
    delay(500);
  });

  Server.begin();

  Cayenne.begin(cayUser, cayPass, cayID, ssid, password);
}

void loop() {
  ArduinoOTA.handle();
  Server.handleClient();
  Cayenne.loop();
  unsigned long ahora = millis() - tiempo_anterior;
  if(ahora > intervaloLed){
    digitalWrite(pinLed, !digitalRead(pinLed));
    tiempo_anterior = millis();
  }
  
}

CAYENNE_IN(canal){
  int valor = getValue.asInt();
  CAYENNE_LOG("Canal %d, pin %d, valor %d", canal, pinRele, valor);
  digitalWrite(pinRele, valor);
  Cayenne.virtualWrite(canal, valor, TYPE_DIGITAL_SENSOR, UNIT_DIGITAL);
}
