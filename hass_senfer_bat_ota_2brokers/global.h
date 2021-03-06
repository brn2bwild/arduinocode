#ifndef GLOBAL_H
#define GLOBAL_H

const int pinLed = 2;
const int pinBoton = 12;
const int pinSensores = 14;

const unsigned int intervaloLedOTA = 2000;
const unsigned int intervaloLedAP = 500;
unsigned long tiempo_anterior_led = 0;

const unsigned int intervaloSensor = 5000; 
unsigned long tiempo_anterior_sensor = 0;

bool boton_presionado_inicio = false;
bool wifi_ota = false;
bool wifi_ap = false;

WiFiClient espClient_lan;
WiFiClient espClient_wan;
PubSubClient ClientLAN(espClient_lan);
PubSubClient ClientWAN(espClient_wan);
ESP8266WebServer Server(80);
OneWire oneWire(pinSensores);
DallasTemperature sondasTemp(&oneWire);

struct strConfig{
  char ssid[30];
  char password[30];
  char MAC[18];
  boolean configurado;
  char dispID[30];
  char serverMQTTLAN[30];
  unsigned int puertoMQTTLAN = 1883;
  char userMQTTLAN[30];
  char passMQTTLAN[30];
  char serverMQTTWAN[30];
  unsigned int puertoMQTTWAN = 1883;
  char userMQTTWAN[30];
  char passMQTTWAN[30];
  char baseTopicos[50];
} config;

void configurarAP(){
  Serial.println("Configurando AP");
  char nombreAPChar[13];
  uint8_t macAP[6];
  WiFi.mode(WIFI_AP);
  WiFi.softAPmacAddress(macAP);
  snprintf(nombreAPChar, 13, "IoT_dev%02X:%02X", macAP[4], macAP[5]);
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
  StringToEEPROM(config.serverMQTTLAN, sizeof(config.serverMQTTLAN), 100);
  StringToEEPROM(config.userMQTTLAN, sizeof(config.userMQTTLAN), 132);
  StringToEEPROM(config.passMQTTLAN, sizeof(config.passMQTTLAN), 164);
  IntToEEPROM(config.puertoMQTTLAN, 196);
  StringToEEPROM(config.serverMQTTWAN, sizeof(config.serverMQTTWAN), 198);
  StringToEEPROM(config.userMQTTWAN, sizeof(config.userMQTTWAN), 230);
  StringToEEPROM(config.passMQTTWAN, sizeof(config.passMQTTWAN), 262);
  IntToEEPROM(config.puertoMQTTWAN, 294);
  EEPROM.commit();
}

boolean leerConfig(){
  Serial.println("Leyendo configuracion");
  if(EEPROM.read(0) == 'C' && EEPROM.read(1) == 'F' && EEPROM.read(2) == 'G'){
    Serial.println("Configuracion encontrada");
    EEPROMToString(config.ssid, sizeof(config.ssid), 4);
    EEPROMToString(config.password, sizeof(config.password), 36);
    EEPROMToString(config.dispID, sizeof(config.dispID), 68);
    EEPROMToString(config.serverMQTTLAN, sizeof(config.serverMQTTLAN), 100);
    EEPROMToString(config.userMQTTLAN, sizeof(config.userMQTTLAN), 132);
    EEPROMToString(config.passMQTTLAN, sizeof(config.passMQTTLAN), 164);
    EEPROMToInt(config.puertoMQTTLAN, 196);
    EEPROMToString(config.serverMQTTWAN, sizeof(config.serverMQTTWAN), 198);
    EEPROMToString(config.userMQTTWAN, sizeof(config.userMQTTWAN), 230);
    EEPROMToString(config.passMQTTWAN, sizeof(config.passMQTTWAN), 262);
    EEPROMToInt(config.puertoMQTTWAN, 294);
    snprintf(config.baseTopicos, 50, "data/%s/", config.dispID);
    // snprintf(config.topicoSondaB, 50, "data/%s/temSondaB", config.dispID);
    // snprintf(config.topicoSondaC, 50, "data/%s/temSondaC", config.dispID);
    // snprintf(config.topicoBateria, 50, "data/%s/porcenBat", config.dispID);
    // snprintf(config.topicoTest, 50, "data/%s/test", config.dispID);
    return true;
  }else{
    Serial.println("No se encontro configuracion");
    return false;
  }
}

void borrarConfig(){
  Serial.println("Borrando configuracion");
  config.configurado = false;
  for(int i = 0; i < 296; i++) EEPROM.write(0, i);
  EEPROM.commit();
}

#endif