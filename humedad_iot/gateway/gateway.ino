#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
// #include <OneWire.h>
// #include <DallasTemperature.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
// #include <RH_ASK.h>
// #include <SPI.h>
#include "funciones.h"
#include "global.h"
#include "pagina_admin.h"
#include "pagina_red.h"
#include "pagina_general.h"
#include "pagina_info.h"
// #include "pagina_datos.h"
#include "pagina_css.h"
#include "pagina_js.h"

void setup() {
  pinMode(pinLed, OUTPUT);
  pinMode(pinBoton, INPUT);
  digitalWrite(pinLed, HIGH);

  if(!digitalRead(pinBoton)) boton_presionado_inicio = true;

  EEPROM.begin(512);
  Serial.begin(9600);
  Serial.setTimeout(200);
  while(!Serial);
  Serial.println("\nIniciando");

  // Serial.println("Configurando sondas de temperatura...");
  // sondasTemp.begin();

  obtenerMAC(config.MAC, sizeof(config.MAC));
  config.configurado = leerConfig();

  // Serial.println(config.clave);

  // if(config.configurado){
    
  //   //se necesita establecer un codigo de parpadeos del led de estado para saber
  //   //de manera visual en que modo está operando el dispositivo
  // }

  if(boton_presionado_inicio){
    
    unsigned long tiempo_inicio_boton = millis();
    byte contador_boton = 0;
    byte estado_boton = 0;
    byte ultimo_estado_boton = 0;

    while(1){
      digitalWrite(pinLed, !digitalRead(pinLed));
      estado_boton = digitalRead(pinBoton);
      if((estado_boton != ultimo_estado_boton) && (estado_boton == 0)) contador_boton++;
      ultimo_estado_boton = estado_boton;
      if((unsigned long)(millis() - tiempo_inicio_boton) >= 10000) break;
      delay(50);
    }

    if(contador_boton == 2 && config.configurado) wifi_ota = true;
    else if(contador_boton == 4) wifi_ap = true;
    else if(contador_boton == 6){
      borrarConfig();
      digitalWrite(pinLed, HIGH);
      Serial.println("Configuracion borrada, se reiniciara el dispositivo en 5s");
      delay(5000);
      ESP.restart();
      // ESP.deepSleep(2e6);
    }else{
      Serial.println("Comando no indicado");
      ESP.restart();
      // ESP.deepSleep(5e6);
    }
      
    if(wifi_ap) configurarAP();
  }

  if(config.configurado && !wifi_ap){
    WiFi.mode(WIFI_STA);

    if(!configurarWiFi()){
      Serial.println("Datos de la red WiFi incorrectos");
      Serial.println("Dispositivo se reiniciara en 5s...");
      digitalWrite(pinLed, HIGH);
      delay(5000);
      ESP.restart();
      // ESP.deepSleep(5e6);
    }

    Serial.print("IP: ");Serial.println(WiFi.localIP());

    configurarOTA();

    Client.setServer(config.serverMQTT, config.puertoMQTT);
    Client.setCallback(callback);

    if(Client.connect(config.MAC, config.userMQTT, config.passMQTT, config.topicoStatus, 0, 1, "off")){
      Client.subscribe(config.topicoTest);
      Client.publish(config.topicoStatus, "on", true);
      Serial.println("Dispositivo conectado al servidor...");
    }else{
      Serial.println("Error al establecer conexión con el servidor...");
    }
  }
  
  Server.on("/", [](){
    Serial.println("admin.html");
    Server.send(200, "text/html", adminPAGE);
    delay(500);
  });

  Server.on("/favicon.ico", [](){
    Serial.println("favicon.ico");
    Server.send(200, "text/html", "");
  });
    
  Server.on("/config.html", send_network_configuration_html);
  Server.on("/general.html", send_general_html);

  Server.on("/info.html", [](){
    Serial.println("/info.html");
    Server.send(200, "text/html", infoPAGE);
  });

  // Server.on("/datos.html", [](){
  //   Serial.println("datos.html");
  //   Server.send(200, "text/html", datosPAGE);
  // });

  Server.on("/style.css", [](){
    Serial.println("style.css");
    Server.send(200, "text/plain", cssPAGE);
  });

  Server.on("/microajax.js", [](){
    Serial.println("microajax.js");
    Server.send(200, "text/plain", microajaxPAGE);
  });

  Server.on("/admin/valores", send_network_configuration_values_html);
  Server.on("/admin/estadoconexion", send_connection_state_values_html);
  Server.on("/admin/valoresgenerales", send_general_configuration_values_html);
  Server.on("/admin/devicename", send_devicename_value_html);
  Server.on("/admin/valoresinfo", send_information_values_html);
  // Server.on("/admin/valoressensores", send_data_html);

  Server.onNotFound([](){
    Serial.println("Página no encontrada");
    Server.send(400, "text/html", "Página no encontrada");
  });

  Server.begin();
  Serial.println("Servidor iniciado");
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido [");Serial.print(topic);Serial.print("] ");
  for (int i=0;i<length;i++)Serial.print((char)payload[i]);
  Serial.println();
}

void enviarDatos(char arreglo[]){
  if(Client.state() == 0){
    Client.publish(config.topicoGateway, arreglo, true);
    Serial.println("Datos enviados");
  }else if(Client.connect(config.MAC, config.userMQTT, config.passMQTT, config.topicoStatus, 0, 1, "off")){
    Client.subscribe(config.topicoTest);
    Client.publish(config.topicoStatus, "on", true);
    Serial.println("Dispositivo reconectado al servidor");
    Client.publish(config.topicoGateway, arreglo, true);
    Serial.println("Datos enviados");
  }else{
    Serial.print("Error en la conexion con el servidor: ");
    Serial.println(Client.state());
  }
}

void loop() {
  unsigned long ahora = millis();
  
  Server.handleClient();

  if(wifi_ota){
    ArduinoOTA.handle();
    if((unsigned long)(ahora - tiempo_anterior_led) >= intervaloLedOTA){
      digitalWrite(pinLed, !digitalRead(pinLed));
      tiempo_anterior_led = millis();
    }
  }else if(wifi_ap){
    if((unsigned long)(ahora - tiempo_anterior_led) >= intervaloLedAP){
      digitalWrite(pinLed, !digitalRead(pinLed));
      tiempo_anterior_led = millis();
    }  
  }else{
    Client.loop();

    if(Serial.available()){
      char aux = Serial.read();
      if(aux != '\n'){
        cadenaSerial[contadorSerial] = aux;
        contadorSerial++;
        if(contadorSerial >= sizeof(cadenaSerial)) contadorSerial = 0;
      }else{
        if(cadenaSerial[0] == '{') bDatoslistos = true;
        contadorSerial = 0;
      }
    }

    if(bDatoslistos){
      // Serial.println(cadenaSerial);
      StaticJsonBuffer<100> JsonBuffer;
      JsonObject& datosJson = JsonBuffer.parseObject(cadenaSerial);
      char encriptacion[32];
      md5.begin();
      md5.add(config.dispID);
      md5.add(config.clave);
      md5.calculate();
      md5.getChars(encriptacion);
      // Serial.println(encriptacion);

      if(!datosJson.success()){
        Serial.println("Error al recibir informacion- JSON");
      }else if(strcmp(encriptacion, datosJson["c"]) == 0){
        const char* equipo = datosJson["e"];
        float humedad = datosJson["h"];
        int bateria = datosJson["b"];
        char cadenamqtt[60];
        snprintf(cadenamqtt, 60, "{\"equipo\":\"%s\",\"humedad\":%.1f,\"bateria\":%d}", equipo, humedad, bateria);
        enviarDatos(cadenamqtt);
      }else{
        Serial.println("Error al recibir informacion- hash incorrecto");
      }
      bDatoslistos = false;
      Serial.flush();
    }

    if((unsigned long)(ahora - tiempo_anterior_led) >= intervaloLedNormal){
      digitalWrite(pinLed, !digitalRead(pinLed));
      tiempo_anterior_led = millis();
    }
  }
}
