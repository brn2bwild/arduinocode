#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>
#include <EEPROM.h>
#include "funciones.h"
#include "global.h"
#include "pagina_admin.h"
#include "pagina_red.h"
#include "pagina_general.h"
#include "pagina_info.h"
#include "pagina_datos.h"
#include "pagina_css.h"
#include "pagina_js.h"

void setup() {
  pinMode(pinLed, OUTPUT);
  pinMode(pinBoton, INPUT);
  digitalWrite(pinLed, HIGH);

  if(!digitalRead(pinBoton)) boton_presionado_inicio = true;

  EEPROM.begin(512);
  Serial.begin(115200);
  Serial.setTimeout(200);
  while(!Serial);
  Serial.println("\nIniciando");

  Serial.println("Configurando sondas de temperatura...");
  sondasTemp.begin();

  obtenerMAC(config.MAC, sizeof(config.MAC));

  config.configurado = leerConfig();

  if(config.configurado){
    ClientLAN.setServer(config.serverMQTTLAN, config.puertoMQTTLAN);
    ClientLAN.setCallback(callback);

    // ClientWAN.setServer(config.serverMQTTWAN, config.puertoMQTTWAN);
    // ClientWAN.setCallback(callback);

    // Serial.println(config.userMQTTLAN);Serial.println(config.passMQTTLAN);

    //se necesita establecer un codigo de parpadeos del led de estado para saber
    //de manera visual en que modo está operando el dispositivo
  }

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
      ESP.deepSleep(2e6);
    }else{
      Serial.println("Comando no indicado");
      ESP.deepSleep(5e6);
    }
      

    if(wifi_ota){
      WiFi.mode(WIFI_STA);

      if(!configurarWiFi()) ESP.deepSleep(5e6);

      Serial.print("IP: ");Serial.println(WiFi.localIP());

      configurarOTA();
  
      if(ClientLAN.connect(config.MAC, config.userMQTTLAN, config.passMQTTLAN)){
        char topico[65];
        snprintf(topico, 65, "%stest", config.baseTopicos);
        if(ClientLAN.subscribe(topico)) Serial.println(topico);
      }else{
        Serial.println("Error al establecer conexión con el servidor...");
      }
    }else if(wifi_ap){
      configurarAP();
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

    Server.on("/datos.html", [](){
      Serial.println("datos.html");
      Server.send(200, "text/html", datosPAGE);
    });

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
    Server.on("/admin/valoressensores", send_data_html);

    Server.onNotFound([](){
      Serial.println("Página no encontrada");
      Server.send(400, "text/html", "Página no encontrada");
    });

    Server.begin();
    Serial.println("Servidor iniciado");
  }else{
    if(config.configurado){
      WiFi.mode(WIFI_STA);

      if(!configurarWiFi()) ESP.deepSleep(5e6);
      Serial.print("IP: ");Serial.println(WiFi.localIP());

      enviarDatos();
      
      ESP.deepSleep(1800e6); // tiempo de 15 min para volver a mandar datos
    }else{
      Serial.println("Dispositivo no configurado, se reiniciará en 5s");
      ESP.deepSleep(5e6);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido [");Serial.print(topic);Serial.print("] ");
  for (int i=0;i<length;i++)Serial.print((char)payload[i]);
  Serial.println();
}

void enviarDatos(){  
  if(ClientLAN.connect(config.MAC, config.userMQTTLAN, config.passMQTTLAN)){
    char dato[8];
    char topico[65];
    sondasTemp.requestTemperatures();

    snprintf(topico, 65, "%stemSondaA", config.baseTopicos);
    snprintf(dato, 8, "%.1f", sondasTemp.getTempCByIndex(0));
    ClientLAN.publish(topico, dato, true);

    snprintf(topico, 65, "%stemSondaB", config.baseTopicos);
    snprintf(dato, 8, "%.1f", sondasTemp.getTempCByIndex(1));
    ClientLAN.publish(topico, dato, true);

    snprintf(topico, 65, "%stemSondaC", config.baseTopicos);
    snprintf(dato, 8, "%.1f", sondasTemp.getTempCByIndex(2));
    ClientLAN.publish(topico, dato, true);

    snprintf(topico, 65, "%sporcenBat", config.baseTopicos);
    snprintf(dato, 8, "%d", obtenerVBAT());
    ClientLAN.publish(topico, dato, true);

    if(!wifi_ota) ClientLAN.disconnect();
    Serial.println("Datos enviados");
  }else{
    Serial.println("Server MQTT sin conexión");
  }
}

void loop() {
  unsigned long ahora = millis();

  if(wifi_ota){
    ArduinoOTA.handle();
  
    ClientLAN.loop();

    if((unsigned long)(ahora - tiempo_anterior_sensor) >= intervaloSensor){
      enviarDatos();
      tiempo_anterior_sensor = millis();
    }

    if((unsigned long)(ahora - tiempo_anterior_led) >= intervaloLedOTA){
      digitalWrite(pinLed, !digitalRead(pinLed));
      tiempo_anterior_led = millis();
    }
  }else if(wifi_ap){
    if((unsigned long)(ahora - tiempo_anterior_led) >= intervaloLedAP){
      digitalWrite(pinLed, !digitalRead(pinLed));
      tiempo_anterior_led = millis();
    }  
  }
  Server.handleClient();
}
