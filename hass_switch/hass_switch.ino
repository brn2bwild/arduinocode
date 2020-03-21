//#include <CayenneMQTTESP8266.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <FS.h>

#define pinLed    13
#define pinBoton  0 
#define pinRele   12 //sonoff 12

#define estadoON "ON"
#define estadoOFF "OFF"

#define intervaloVerificacionServidor 60000
#define intParpadeoNoOk 200

unsigned long verificarServidor;
unsigned long tiempoParpadeoAnterior;

const int mqttport = 1883;

uint8_t MAC_AP[WL_MAC_ADDR_LENGTH];// variable que guardará la MAC_AP del dispositivo
uint8_t MAC_WIFI[6];
char mac[20], topicoComando[50], topicoEstado[50];

bool estadoWiFi;

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
          margin: 0 0 5px 0;
          border: 1.5px solid #000;
          border-radius: 5px;
          text-align: center;
          margin: 0 0 1% 0;
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
     .btnswitch{
          font-size: 1.6rem;
          width: 75%;
          cursor: pointer;
          border-radius: 5px;
          margin: 0 0 1% 0;
     }
</style>
<meta name='viewport' content='width=device-width, initial-scale=1.0'>
<meta charset='utf-8'>  
<title>IoT Device</title>
</head>
<body>
     <h1 style="margin: 0">Switch-IoT</h1>
     <h2>Introduzca las credenciales</h2>
     <button class='btnswitch' onclick="toggle()">Encender/apagar</button>
     <input class='input' id='ssid' placeholder="Red WiFi" maxlength="50">
     <input type="password" class='input' id='pass' placeholder="WiFi password" maxlength="50">
     <input class='input' id='mqttserver' placeholder="Server" maxlength="50">
     <input class="input" id="mqttuser" placeholder="MQTT user" maxlength="50">
     <input type="password" class="input" id="mqttpass" placeholder="MQTT password" maxlength="50">
     <input class="input" id="mqttid" placeholder="ID dispositivo" maxlength="50">
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
     function toggle(){
          console.log("Cambio interruptor...");
          var xhr = new XMLHttpRequest();
          var url = "/toggle";

          xhr.onreadystatechange = function(){
               if(this.readyState == 4 && this.status == 200){
                    if(xhr.responseText != null){
                         console.log(xhr.responseText);
                    }
               }
          }

          xhr.open("POST", url, true);
          xhr.send();
     }
</script>
</html>
)=====";

WiFiClient espClient;
PubSubClient client(espClient);
ESP8266WebServer server;

void guardarDatoswifi(){
  String data = server.arg("plain");
  DynamicJsonBuffer jBuffer;
  JsonObject& jObject = jBuffer.parseObject(data);

  File configFile = SPIFFS.open("/config.json","w");
  jObject.printTo(configFile);
  configFile.close();

  server.send(200, "application/json", "{\"estado\":\"guardando datos\",\"WiFi\":\"conectando\"}");
  delay(500);

  conectar_WiFi();
}

void callback(char* topic, byte* payload, unsigned int length){
  char strPayload[sizeof(payload)];
  snprintf(strPayload, sizeof(strPayload), "%s", payload);
  if(strcmp(topicoComando, topic) == 0){
    if(strcmp(strPayload, estadoON) == 0){
      toggleRele();
      client.publish(topicoEstado, "ON");
    }else if(strcmp(strPayload, estadoOFF) == 0){
      toggleRele();
      client.publish(topicoEstado, "OFF");
    }
  }
}

void toggleRele(){
  digitalWrite(pinRele, !digitalRead(pinRele));
  server.send(200, "application/json", "{\"estado\":\"cambio estado led\"}");
  delay(500);
}

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
  delay(300);

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

        Serial.print("\nIntentado conectar a red WiFi: ");
        Serial.println(_ssid);
        unsigned long tiempoInicio = millis();
        while(WiFi.status() != WL_CONNECTED){
          delay(500);
          Serial.print(".");
          digitalWrite(pinLed, !digitalRead(pinLed));
          if((unsigned long)(millis() - tiempoInicio >= 10000)) break;
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

bool conectar_Servidor(){
  if(!client.connected()){
    if(SPIFFS.exists("/config.json")){
      const char *_mqttserver = "", *_mqttuser = "", *_mqttpass = "", *_mqttid= "";
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
          
          snprintf(topicoComando, 50, "cmnd/%s/power", _mqttid);
          snprintf(topicoEstado, 50, "stat/%s/POWER", _mqttid);

          client.setServer(_mqttserver, mqttport);
          client.setCallback(callback);
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

void setup() {
  
  pinMode(pinRele, OUTPUT);
  pinMode(pinLed, OUTPUT);
  pinMode(pinBoton, INPUT);
  digitalWrite(pinRele, HIGH);

  SPIFFS.begin();
  delay(200);

  Serial.begin(115200);
  estadoWiFi = conectar_WiFi();

  if(estadoWiFi){
    // se inicializa la comunicación con servidor
    Serial.println("WiFi conectado, intentando conectar con el servidor");
    Serial.print("Id: ");
    Serial.println(WiFi.macAddress());
    Serial.print("Ip: ");
    Serial.println(WiFi.localIP());
    if(conectar_Servidor()){
      Serial.println("Dispositivo conectado con el servidor");
    }else{
      Serial.println("Error al conectarse con el servidor");
    }
  }else{
    conectar_AP();
    Serial.print("IP AP: ");
    Serial.println(ipEstatica);
  }
  WiFi.printDiag(Serial);

  server.on("/",[](){server.send_P(200,"text/html",pagina_web);});
  server.on("/toggle", HTTP_POST, toggleRele);
  server.on("/config", HTTP_POST, guardarDatoswifi);
  server.on("/reset", [](){
    server.send(200,"application/json","{\"estado\":\"reiniciando el dispositivo\"}");
    delay(1000);
    ESP.restart();
  });
  server.begin();
}

void loop() {
  unsigned long ahora = millis();

  if(!digitalRead(pinBoton)){
    delay(300);
    digitalWrite(pinRele, !digitalRead(pinRele));
    digitalWrite(pinLed, LOW);
  }
  if(estadoWiFi){
    if((!client.connected()) && ((unsigned long)(ahora - verificarServidor) > intervaloVerificacionServidor)){
      verificarServidor = ahora;
      if(WiFi.status() == WL_CONNECTED){
        Serial.println("Dispositivo desconectado del servidor, intentando reconectar");
        conectar_Servidor();
      }else{
        Serial.println("Dispositivo desconectado de la red WiFi\nIntentando reconectar...");
        conectar_WiFi();
      }
    }else{
      client.loop();
    }
  }else if((unsigned long)(ahora - tiempoParpadeoAnterior) >= intParpadeoNoOk){
    tiempoParpadeoAnterior = ahora;
    digitalWrite(pinLed, !digitalRead(pinLed));
  }
  server.handleClient();
}