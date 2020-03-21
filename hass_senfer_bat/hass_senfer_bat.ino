#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <SPI.h>
// #include <Adafruit_GFX.h>
// #include <Adafruit_SSD1306.h>
#include <Wire.h>

// #include "images.h"

#define pinLed            2
#define pinSenFlu         5   // d1
#define pinBoton          4   // d2

#define intervaloMensaje      5000
#define intervaloOK           500
#define intervaloNook         200
#define intervaloMostrardatos 5000

#define estadoON  "ON"
#define estadoOFF "OFF"

// #if (SSD1306_LCDHEIGHT != 64)
// #error("Cantidad de pixeles de altura incorrecta, verificar Adafruit_SSD1306.h");
// #endif

// #define OLED_RESET  LED_BUILTIN

const float calibracion = 0.945;
const int mqttPort = 1883;   // puerto del broker mqtt

char mqttServer[50], topicoComando[50], topicoEstado[50], topicoDatosTemSondaA[60], topicoDatosTemSondaB[60], topicoDatosTemSondaC[60], topicoPorcjeBateria[60];

unsigned long tiempoAnterior = 0, tiempoMostrarDatos;
uint8_t contadorDatos = 0;

float temSondaC_A, temSondaC_B, temSondaC_C;
float porc_bateria, volt_bateria;

uint8_t MAC_AP[WL_MAC_ADDR_LENGTH];// variable que guardará la MAC_AP del dispositivo
uint8_t MAC_WIFI[6];
char mac[20];
bool estadoWiFi, habilitarDispositivo = true;

IPAddress ipEstatica(1,1,1,1);
IPAddress ipMascara(255,255,255,0);

char pagina_web[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html>
<head>
<style>
     html,body{
          width: 100%;
          height: 100%;
          margin: 0 auto;
          font-family: sans-serif;
          box-sizing: border-box;
          text-align: center;
     }
     .input{
          width:75%;
          font-size:26px;
          margin: 0 0 2% 0;
          border: 1.5px solid #000;
          border-radius: 5px;
          text-align: center;
     }
     ::-webkit-input-placeholder{
          color: rgba(0,0,0,0.85);
     }
     .btnguardar{
          font-weight: 600;
          font-size: 2.1rem;
          width: 75%;
          cursor: pointer;
          border-radius: 5px;
     }
</style>
<meta name='viewport' content='width=device-width, initial-scale=1.0' charset="utf-8">
<title>IoT Device</title>
</head>
<body>
     <h1 style="margin: 0">Sensor-IoT</h1>
     <h2>Introduzca las credenciales</h2>
     <input class='input' id='ssid' placeholder="Red WiFi" maxlength=40>
     <input type="password" class='input' id='pass' placeholder="WiFi password" maxlength=40>
     <input class='input' id='mqttserver' placeholder="Server" maxlength="50">
     <input class="input" id="mqttuser" placeholder="MQTT user" maxlength=40>
     <input type="password" class="input" id="mqttpass" placeholder="MQTT password" maxlength=40>
     <input class="input" id="mqttid" placeholder="ID dispositivo">
     <button class='btnguardar' onclick="guardar()">GUARDAR</button>
</body>
</form>
<script type="text/javascript">
     function guardar(){
          console.log("Guardando datos...");

          var ssid = document.getElementById('ssid').value;
          var pass = document.getElementById('pass').value;
          var mqttserver = document.getElementById('mqttserver').value;
          var mqttuser = document.getElementById('mqttuser').value;
          var mqttpass = document.getElementById('mqttpass').value;
          var mqttid = document.getElementById('mqttid').value;
          var data = {ssid:ssid, pass:pass, mqttserver:mqttserver, mqttuser:mqttuser, mqttpass:mqttpass, mqttid:mqttid};

          console.log(data);

          var xhr = new XMLHttpRequest();
          var url = "/config";

          xhr.onreadystatechange = function(){
               if(this.readyState == 4 && this.status == 200){
                    if(xhr.responseText != null){
                         console.log(xhr.responseText);
                    }
               }
          }

          xhr.open("POST", url, true);
          xhr.send(JSON.stringify(data));

          alert("Datos cargados, reiniciar dipositivo en 3s");
     };
</script>
</html>
)=====";
int codStatus = 0;

//se inicializan las instancias que se encargarán de las conexiones WiFi, MQTT y el server AP
WiFiClient espClient;
PubSubClient client(espClient);
ESP8266WebServer server;

//se inicializan las instancias que se encargarán de los sensores
//Adafruit_SSD1306 display(OLED_RESET);         //se inicializa la instancia de la pantalla oled

OneWire oneWire(pinSenFlu);                   //se inicializa la instancia del sensor de temperatura SD18B20
DallasTemperature sondaTemperatura(&oneWire);     

void conectar_AP(){
  WiFi.mode(WIFI_AP);
  WiFi.softAPmacAddress(MAC_AP);

  String macDisp = String(MAC_AP[WL_MAC_ADDR_LENGTH - 2], HEX) + String(MAC_AP[WL_MAC_ADDR_LENGTH - 1], HEX);
  macDisp.toUpperCase();

  String nombreAP = "IOT_dev" + macDisp;

  Serial.println(nombreAP);

  char nombreAPChar[nombreAP.length() + 1];

  memset(nombreAPChar, 0, nombreAP.length() + 1);

  for(int i = 0; i < nombreAP.length(); i++){
    nombreAPChar[i] = nombreAP.charAt(i);
  }

  const char* passAPchar = "012345678";

  WiFi.softAPConfig(ipEstatica, ipEstatica, ipMascara);
  WiFi.softAP(nombreAPChar, passAPchar);
  digitalWrite(pinLed, LOW);
}

bool conectar_WiFi(){
  WiFi.softAPdisconnect(true);
  WiFi.disconnect();
  delay(1000);

  if(SPIFFS.exists("/config.json")){
    const char *_ssid = "", *_pass = "";
    File configFile = SPIFFS.open("/config.json","r");
    if(configFile){
      size_t size = configFile.size();
      std::unique_ptr<char[]> buf(new char[size]);
      configFile.readBytes(buf.get(), size);
      configFile.close();

      DynamicJsonBuffer jsonBuffer;
      JsonObject &jObject = jsonBuffer.parseObject(buf.get());
      if(jObject.success()){
        _ssid = jObject["ssid"];
        _pass = jObject["pass"];

        WiFi.mode(WIFI_STA);
        WiFi.begin(_ssid, _pass);

        Serial.print("\nIntentado conectar a red WiFi: "); Serial.println(_ssid);
        unsigned long tiempoInicio = millis();
        while(WiFi.status() != WL_CONNECTED){
          delay(500);
          Serial.print(".");
          digitalWrite(pinLed, !digitalRead(pinLed));
          if((unsigned long)(millis() - tiempoInicio) >= 20000) break;
        }
      }
    }
  }

  if(WiFi.status() == WL_CONNECTED){
    return true;
  }else{
    return false;
  }
}

void guardarDatoswifi(){
  String data = server.arg("plain");
  DynamicJsonBuffer jBuffer;
  JsonObject& jObject = jBuffer.parseObject(data);

  File configFile = SPIFFS.open("/config.json","w");
  jObject.printTo(configFile);
  configFile.close();

  server.send(200, "application/json", "{\"estado\":\"ok\"}");
  delay(500);

  conectar_WiFi();
}

void callback(char* topic, byte* payload, unsigned int length) {
  char strPayload[sizeof(payload)];
  snprintf(strPayload, sizeof(strPayload), "%s", payload);
  if(strcmp(topicoComando, topic) == 0){
    if(strcmp(strPayload, estadoON) == 0){
      habilitarDispositivo = true;
      client.publish(topicoEstado,"ON");
    }else if(strcmp(strPayload, estadoOFF) == 0){
      habilitarDispositivo = false;
      client.publish(topicoEstado,"OFF");
    }
  }
}

bool conectar_Servidor() {
  // Se intenta reconectar al servidor
  if (!client.connected()) {
    if(SPIFFS.exists("/config.json")){
      const char *_mqttserver = "", *_mqttuser = "", *_mqttpass = "", *_mqttid = "";
      File configFile = SPIFFS.open("/config.json","r");
      if(configFile){

        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        configFile.close();

        DynamicJsonBuffer jsonBuffer;
        JsonObject &jObject = jsonBuffer.parseObject(buf.get());
        if(jObject.success()){

          _mqttserver = jObject["mqttserver"];
          _mqttuser = jObject["mqttuser"];
          _mqttpass = jObject["mqttpass"];
          _mqttid = jObject["mqttid"];

          Serial.println(_mqttserver);
          Serial.println(_mqttuser);
          Serial.println(_mqttpass);
          Serial.println(_mqttid);
          snprintf(mqttServer, 50, "%s", _mqttserver);
          snprintf(topicoComando, 50, "cmnd/%s/power", _mqttid);
          snprintf(topicoEstado, 50, "stat/%s/POWER", _mqttid);
          snprintf(topicoDatosTemSondaA, 60, "data/%s/temSondaA", _mqttid);
          snprintf(topicoDatosTemSondaB, 60, "data/%s/temSondaB", _mqttid);
          snprintf(topicoDatosTemSondaC, 60, "data/%s/temSondaC", _mqttid);
          snprintf(topicoPorcjeBateria, 60, "data/%s/porcenBat", _mqttid);

          client.setServer(_mqttserver, mqttPort);
          client.setCallback(callback);
          WiFi.macAddress(MAC_WIFI);
          snprintf(mac, 20, "%02X:%02X:%02X:%02X:%02X:%02X", MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5]);

          if(client.connect(mac, _mqttuser, _mqttpass)){
            Serial.println("Dispositivo conectado con el servidor");
            Serial.print("Server: ");
            Serial.println(_mqttserver);
            if(client.subscribe(topicoComando)){
              Serial.println("Dispositivo suscrito topico de comandos");
              digitalWrite(pinLed, LOW);
            }else{
              Serial.println("Error al suscribirse al topico de comandos");
            }
            return true;
          }else{
            return false;
          }
        }
      }
    }else{
      Serial.println("Error al leer las credenciales de la memoria FLASH");
      return false;
    }
  }
}

void leerDatos(){
   sondaTemperatura.requestTemperatures(); 
   temSondaC_A = sondaTemperatura.getTempCByIndex(0); 
   temSondaC_B = sondaTemperatura.getTempCByIndex(1); 
   temSondaC_C = sondaTemperatura.getTempCByIndex(2); 
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(1000);
  while(!Serial);

  float volt_bateria = (((analogRead(0)*3.3)/1024)*calibracion);
  Serial.print("Porcentaje de batería: ");
  float porc_bateria = (voltaje*100.0)/2.31;
  Serial.println(porc_bateria);
  
  if(porc_bateria < 60.0){
    Serial.println("Bajo voltaje de la batería, recargar");
    ESP.deepSleep(5e6);
  }

  Serial.println("\nConfigurando entradas y salidas...");
  pinMode(pinLed, OUTPUT);
  pinMode(pinBoton, INPUT);

  Serial.println("Inicializando sondas de temperatura...");
  sondaTemperatura.begin(); //éstas líneas se tienen que comentar para probar el dispositivo sin sensores
  delay(200);

  SPIFFS.begin();
  delay(200);

  if(digitalRead(pinBoton)){
    estadoWiFi = conectar_WiFi();
    if(!estadoWiFi){
      Serial.println("\nError al conectarse a la red WiFi");
      ESP.deepSleep(5e6);
    }

    Serial.print("\nDispositivo conectado a la red WiFi - ");
    Serial.print("Ip: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC: ");
    Serial.println(mac);
    WiFi.printDiag(Serial);

    if(!conectar_Servidor()){    
      Serial.println("Error al conectar el dispositivo con el servidor");
      ESP.deepSleep(5e6);
    }

    Serial.println("Dispositivo conectado con el servidor\nLeyendo datos");

    leerDatos(); //éstas líneas se tienen que comentar para probar el dispositivo sin sensores

    char dato[8];

    dtostrf(temSondaC_A, 6, 2, dato);
    client.publish(topicoDatosTemSondaA, dato);
    dtostrf(temSondaC_B, 6, 2, dato);
    client.publish(topicoDatosTemSondaB, dato);
    dtostrf(temSondaC_C, 6, 2, dato);
    client.publish(topicoDatosTemSondaC, dato);

    Serial.print(temSondaC_A);
    Serial.print(" - ");
    Serial.print(temSondaC_B);
    Serial.print(" - ");
    Serial.println(temSondaC_C);
    Serial.println("Datos enviados");
    ESP.deepSleep(30e6);
  }

  estadoWiFi = conectar_WiFi();
  if(!estadoWiFi){
    Serial.println("Error al conectar el dispositivo con la red WiFi");
    conectar_AP();
    Serial.print("Ip - AP: ");
    Serial.println(ipEstatica);
  }else{
    Serial.print("\nDispositivo conectado a la red WiFi - ");
    Serial.print("Ip: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC: ");
    Serial.println(mac);
  }

  WiFi.printDiag(Serial);

  if(estadoWiFi && !conectar_Servidor()){
    Serial.println("Error al conectar el dispositivo con el servidor");
  }

  Serial.println("Dispositivo conectado con el servidor");

  server.on("/",[](){server.send_P(200,"text/html", pagina_web);});
  server.on("/config", HTTP_POST, guardarDatoswifi);
  server.on("/reset",[](){
    server.send(200,"application/json","{\"estado\":\"reiniciando el dispositivo\"}");
    delay(1000);
    ESP.restart();
  });
  server.begin();
}

void loop() {
  server.handleClient();
}