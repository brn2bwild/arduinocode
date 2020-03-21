#ifndef GLOBAL_H
#define GLOBAL_H

const int pinLed = 2;
const int pinBoton = 12; //D6
const int pinSensores = 14; //D5
const int pinEnableRF = 13; //D7

const unsigned int intervaloLedOTA = 2000;
const unsigned int intervaloLedAP = 500;
unsigned long tiempo_anterior_led = 0;

const unsigned int intervaloSensor = 10000; 
unsigned long tiempo_anterior_sensor = 0;

const float multiplicadorADS = 0.125;

bool boton_presionado_inicio = false;
bool wifi_ota = false;
bool wifi_ap = false;

// WiFiClient espClient;
// PubSubClient Client(espClient);
ESP8266WebServer Server(80);
MD5Builder md5;
// OneWire oneWire(pinSensores);
// DallasTemperature sondasTemp(&oneWire);
// RH_ASK controlador_rf(2000, 15, 3); // el pin 15-D8 es el Rx y el pin 3-Rx es el Tx

Adafruit_ADS1115 ads;

struct strConfig{
  char ssid[32];
  char password[32];
  char MAC[18];
  boolean configurado;
  char dispID[32];
  char gateway[32];
  char clave[32];
  // unsigned int puertoMQTT = 1883;
  // char userMQTT[30];
  // char passMQTT[30];
  // char topicoGateway[50];
  // char topicoSondaB[50];
  // char topicoSondaC[50];
  // char topicoBateria[50];
  // char topicoTest[50];
} config;

void configurarAP(){
  Serial.println("Configurando AP");
  char nombreAPChar[13];
  uint8_t macAP[6];
  WiFi.mode(WIFI_AP);
  WiFi.softAPmacAddress(macAP);
  snprintf(nombreAPChar, 13, "Nodo-%02X:%02X", macAP[4], macAP[5]);
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
  StringToEEPROM(config.gateway, sizeof(config.gateway), 100);
  StringToEEPROM(config.clave, sizeof(config.clave), 132);

  // StringToEEPROM(config.passMQTT, sizeof(config.passMQTT), 164);
  // IntToEEPROM(config.puertoMQTT, 196);
}

boolean leerConfig(){
  Serial.println("Leyendo configuracion");
  if(EEPROM.read(0) == 'C' && EEPROM.read(1) == 'F' && EEPROM.read(2) == 'G'){
    Serial.println("Configuracion encontrada");
    EEPROMToString(config.ssid, sizeof(config.ssid), 4);
    EEPROMToString(config.password, sizeof(config.password), 36);
    EEPROMToString(config.dispID, sizeof(config.dispID), 68);
    EEPROMToString(config.gateway, sizeof(config.gateway), 100);
    EEPROMToString(config.clave, sizeof(config.clave), 132);
    // EEPROMToString(config.passMQTT, sizeof(config.passMQTT), 164);
    // EEPROMToInt(config.puertoMQTT, 196);
    // snprintf(config.topicoGateway, 50, "data/%s", config.dispID);
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
  for(int i = 0; i < 200; i++) EEPROM.write(0, i);
  EEPROM.commit();
}

float obtenerHumedad(){
    // uint16_t valorADC[10];
    // uint16_t aux = 0;
    // uint16_t sumatoria = 0;

    // for (byte i = 0; i < 10; i++){
    //   valorADC[i] = ads.readADC_SingleEnded(0);
    //   delay(5);
    // }

    // for(byte j = 0; j < 9; j++){
    //   if(valorADC[j+1] > valorADC[j]){
    //     aux = valorADC[j];

    //     valorADC[j] = valorADC[j+1];
    //     valorADC[j+1] = aux;
    //   }
    // }
    
    // for(byte k = 2; k < 8; k++) sumatoria += valorADC[k];
    
    // float humedad = (-0.2529*(sumatoria/6.0)) + 214.69;
    uint16_t valorADC = ads.readADC_SingleEnded(0);
    float humedad = (-68.966*(valorADC * multiplicadorADS)/1000.0)+210.34;
    
    if(humedad < 0.0){return 0.0;}
    else if(humedad > 100.0){return 100.0;}
    else{return humedad;}
}

float obtenerVBAT(){
    // int valorADC[10];
    // int aux = 0;
    // int sumatoria = 0;

    // for (byte i = 0; i < 10; i++){
    //   valorADC[i] = analogRead(A0);
    //   delay(5);
    // }

    // for(byte j = 0; j < 9; j++){
    //   if(valorADC[j+1] > valorADC[j]){
    //     aux = valorADC[j];

    //     valorADC[j] = valorADC[j+1];
    //     valorADC[j+1] = aux;
    //   }
    // }

    // for(byte k = 2; k < 8; k++) sumatoria += valorADC[k];

    // return (int)((((0.002 * (sumatoria / 6.0)) + 2.6902) / 4.3) * 100);

    uint16_t valorADC = ads.readADC_Differential_2_3();
    // return (int)((200*(valorADC * multiplicadorADS)/1000.0)-740);
    return (((valorADC * multiplicadorADS)/1000.0)*1.8023);
}

#endif