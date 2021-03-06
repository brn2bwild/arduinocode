#ifndef GLOBAL_H
#define GLOBAL_H

const int pinLed = 2;
const int pinBoton = 12;
const int pinSensores = 14;

const unsigned int intervaloLedOTA = 1000;
const unsigned int intervaloLedAP = 500;
const unsigned int intervaloLedNormal = 5000;
unsigned long tiempo_anterior_led = 0;

const unsigned int intervaloSensor = 5000; 
unsigned long tiempo_anterior_sensor = 0;

bool boton_presionado_inicio = false;
bool wifi_ota = false;
bool wifi_ap = false;

bool estadoActserver = false;
bool estadoAntserver = false;

byte contadorSerial = 0;
bool bDatosentrantes = false, bDatoslistos = false;
char cadenaSerial[70];

WiFiClient espClient;
PubSubClient Client(espClient);
ESP8266WebServer Server(80);
MD5Builder md5;
// OneWire oneWire(pinSensores);
// DallasTemperature sondasTemp(&oneWire);
// RH_ASK controlador_rf(2000, 15, 5); // el pin 15-D8 es el Rx y el pin 5-D1 es el Tx

struct strConfig{
  char ssid[32];
  char password[32];
  char MAC[18];
  boolean configurado;
  char dispID[32];
  char clave[6];
  char serverMQTT[32];
  unsigned int puertoMQTT = 1883;
  char userMQTT[32];
  char passMQTT[32];
  char topicoGateway[50];
  char topicoTest[50];
  char topicoStatus[50];
} config;

void borrarConfig(){
  Serial.println("Borrando configuracion");
  config.configurado = false;
  for(int i = 0; i < 210; i++) EEPROM.write(0, i);
  EEPROM.commit();
}

void configurarAP(){
  Serial.println("Configurando AP");
  char nombreAPChar[13];
  uint8_t macAP[6];
  WiFi.mode(WIFI_AP);
  WiFi.softAPmacAddress(macAP);
  snprintf(nombreAPChar, 13, "Gateway-%02X:%02X", macAP[4], macAP[5]);
  WiFi.softAPConfig(IPAddress(192,168,1,4), IPAddress(192,168,1,1), IPAddress(255,255,255,0));
  WiFi.softAP(nombreAPChar);
}

boolean configurarWiFi(){
    Serial.println("Configurando WiFi");

    //cuando se puede programar por OTA se puede hardcodear las crendeciales del WiFi
    WiFi.begin(config.ssid, config.password);

    byte contadorWiFi = 0;

    while(WiFi.waitForConnectResult() != WL_CONNECTED){
        Serial.print(".");
        if(contadorWiFi++ > 40) break;
        delay(500);        
    }
    Serial.println("");

    if (WiFi.status() == WL_CONNECTED){
        return true;
    }else{
        return false;
    }
}

void configurarOTA(){
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(config.dispID);

  // No authentication by default
  ArduinoOTA.setPassword("admin");

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
    Serial.println("Iniciando actualización " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nFinalizado");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progreso: %u%%\r", (progress / (total / 100)));
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
}

void guardarConfig(){
  Serial.println("Guardando configuracion");

  EEPROM.write(0, 'C'); EEPROM.write(1, 'F'); EEPROM.write(2, 'G');
  StringToEEPROM(config.ssid, sizeof(config.ssid), 4);
  StringToEEPROM(config.password, sizeof(config.password), 36);
  StringToEEPROM(config.dispID, sizeof(config.dispID), 68);
  StringToEEPROM(config.clave, sizeof(config.clave), 100);
  StringToEEPROM(config.serverMQTT, sizeof(config.serverMQTT), 106);
  StringToEEPROM(config.userMQTT, sizeof(config.userMQTT), 138);
  StringToEEPROM(config.passMQTT, sizeof(config.passMQTT), 170);
  IntToEEPROM(config.puertoMQTT, 202);
}

boolean leerConfig(){
  Serial.println("Leyendo configuracion");
  if(EEPROM.read(0) == 'C' && EEPROM.read(1) == 'F' && EEPROM.read(2) == 'G'){
    Serial.println("Configuracion encontrada");
    EEPROMToString(config.ssid,sizeof(config.ssid), 4);
    EEPROMToString(config.password,sizeof(config.password), 36);
    EEPROMToString(config.dispID,sizeof(config.dispID), 68);
    EEPROMToString(config.clave,sizeof(config.clave), 100);
    EEPROMToString(config.serverMQTT,sizeof(config.serverMQTT), 106);
    EEPROMToString(config.userMQTT,sizeof(config.userMQTT), 138);
    EEPROMToString(config.passMQTT,sizeof(config.passMQTT), 170);
    EEPROMToInt(config.puertoMQTT, 202);
    snprintf(config.topicoGateway, 50, "data/%s", config.dispID);
    snprintf(config.topicoTest, 50, "test/%s", config.dispID);
    snprintf(config.topicoStatus, 50, "status/%s", config.dispID);

    return true;
  }else{
    Serial.println("No se encontro configuracion");
    return false;
  }
}

#endif