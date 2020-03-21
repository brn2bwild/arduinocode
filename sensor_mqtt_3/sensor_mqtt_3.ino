/*
 Código de un sensor tipo Ph, el cuál debería ser capáz de medir Ph, temperatura del 
 aire, temperatura de un fluido, humedad del aire, así como también ser capáz de
 calcular la sensación térmica a partir de los valores de temperatura y humedad.
*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <math.h>
#include <time.h>
#include <DFRobot_BME280.h>
#include <FS.h>

#define PRESION_NIVEL_MAR     1013.25f
#define BME_CS                10

#define DISPOSITIVO_CONFIG    0
#define DISPOSITIVO_NOSERVER  1
#define DISPOSITIVO_NOWIFI    2

#define pinLed          2
#define pinTx           14 // d5
#define pinLecturas     12 // d6
#define pinSenFlu       0  // d3
#define pinSenPh        A0 

#define OffsetPh        0.0

//const char* ssid = "Tesalia-AP";
//const char* password = "aahooraa";

const char* mqtt_server = "192.168.10.200";  //se debe configurar la ip del raspberry para que quede estática
//const char* mqtt_server = "iot.eclipse.org";  //se debe configurar la ip del raspberry para que quede estática
const int mqtt_port = 1883;
const char* IDCliente = "keYJw21dQ3g=";   // cadena: stA00001 codificada con algoritmo arcfour en modo CBC clave: sensor
//const char* IDCliente = "keYJw21dQ3s=";     // cadena: sTA00002 codificada con algoritmo arcfour en modo CBC clave: sensor
const char* tipoSensor = "sensor01";

//variables utilizadas para conexión con el servidor MQTT
//char cadenaTopico[100]; // variable que contendrá la ruta del topico al cual se subscribirá el dispositivo
char mensaje[50];

//char ubicacionDispositivo[20] = "alebines";
//char IDCliente[20] = "estanque";
//char IDCliente[20] = "cisterna";

unsigned long ultimoMensaje = 0; //varible que controlará el tiempo de envío de datos
unsigned long ultimoParpadeo = 0;
unsigned char contadorReconexionWiFi = 0;
float valor = 0.0;

//char eeprom_ssid[40];
//char eeprom_pass[40];
uint8_t MAC_AP[WL_MAC_ADDR_LENGTH];// variable que guardará la MAC_AP del dispositivo
uint8_t MAC_WIFI[6];
int estadoWiFi;

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
     }
     .form{
          padding: 25px;
          margin: 0 auto;
          margin: 1% 0 0 0;
          background-color: #f05f40;  
     }
     h1{
          color: #F05F40;
          padding: 4% 0 0 20px;
          font-size: 70px;
          margin: 0 auto;
     }
     h2{
          text-align: center;
          color: #FFFFFF;
          margin: 0 auto;
     }
     .input{
          width:95%;
          font-size:30px;
          margin: 0 0 15px 2.5%;
          border: 1.5px solid #FFF;
          background-color: transparent;
          color: #FFFFFF;
          border-radius: 5px;
          text-align: center;
     }
     ::-webkit-input-placeholder{
          color: rgba(255,255,255,0.5);
     }
     ::-moz-input-placeholder{
          color: rgba(255,255,255,0.5);
     }
     .label{
          color: #ffffff;
          font-size: 30px;
          margin: 0 0 5px 0;
     }
     .boton{
          color: #F05F40;
          background-color: #FFFFFF;
          font-weight: 600;
          font-size: 3rem;
          border: none;
          width: 95%;
          cursor: pointer;
          border-radius: 5px;
          margin: 2% 0 0 2.5%;
     }
</style>
<meta name='viewport' content='width=device-width, initial-scale=1.0'>
<meta charset='utf-8'>  
<title>IOT Device</title>
</head>
<body>
     <h1>Sensor-IOT</h1>
     <div class='form'>
          <h2>Introduzca las credenciales</h2>
          <label for="ssid" class='label'>SSID:</label>
          <input class='input' id='ssid' placeholder="Red WiFi">
          <label for="pass" class='label'>PASS:</label>
          <input type="password" class='input' id='pass' placeholder="Contraseña">
          <button class='boton' onclick="guardar()">GUARDAR</button>
     </div>
</body>
</form>
<script type="text/javascript">
     function guardar(){
          console.log("Guardando datos...");

          var ssid = document.getElementById('ssid').value;
          var pass = document.getElementById('pass').value;
          var data = {ssid:ssid, pass:pass};

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
OneWire oneWire(pinSenFlu);
DallasTemperature sensorFluido(&oneWire);
DFRobot_BME280 bme;

void indicarOrden(char orden){
  char jsonMensaje[70];
  if(orden == 'r'){
    snprintf(jsonMensaje, 70, "{\"disp\":\"%s-%02X:%02X:%02X:%02X:%02X:%02X\",\"estado\":\"reiniciado\"}", tipoSensor, MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5]);
    client.publish("sistema/dispositivos/sensores",jsonMensaje);
    delay(1000);
    ESP.restart();
  }else if(orden == 'd'){
    snprintf(jsonMensaje, 70, "{\"disp\":\"%s-%02X:%02X:%02X:%02X:%02X:%02X\",\"estado\":\"deshabilitado\"}", tipoSensor, MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5]);
  }else if(orden == 'h'){
    snprintf(jsonMensaje, 70, "{\"disp\":\"%s-%02X:%02X:%02X:%02X:%02X:%02X\",\"estado\":\"activo\"}", tipoSensor, MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5]);
  }
  client.publish("sistema/dispositivos/sensores",jsonMensaje);
  delay(1000);
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

int conectar_WiFi(){
  WiFi.softAPdisconnect(true);
  WiFi.disconnect();
  delay(1000);

  if(SPIFFS.exists("/config.json")){
    const char * _ssid = "", *_pass = "";
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

        Serial.println("");
        Serial.println("Intentado conectar a red WiFi");
        unsigned long tiempoInicio = millis();
        while(WiFi.status() != WL_CONNECTED){
          delay(1000);
          Serial.print(".");
          digitalWrite(pinLed, !digitalRead(pinLed));
          if((unsigned long)(millis() - tiempoInicio >= 20000)) break;
        }
      }
    }
  }

  if(WiFi.status() == WL_CONNECTED){
    digitalWrite(pinLed, LOW);
    Serial.println("");
    return 1;
  }else{
    digitalWrite(pinLed, HIGH);
    Serial.println("");
    return -1;
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
  char cadenaReinicio[100];
  memset(cadenaReinicio, 0, sizeof(cadenaReinicio));

  Serial.print("Mensaje recibido [");
  Serial.print(topic);
  Serial.println("]");

  for (int i = 0; i < length; i++) {cadenaReinicio[i] = (char)payload[i];}

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(cadenaReinicio);

  if(!json.success()){Serial.println("Error al recibir comando: ruta de control");}
  const char* idOrden = json["dispositivo"];
  const char* orden = json["orden"];

  if(sizeof(idOrden) > 0){
    int igualID = strcmp(idOrden, IDCliente);

    if(igualID == 0){
      Serial.println(idOrden);
      Serial.println(orden);
      indicarOrden((char)orden[0]);
    }
  }else{
    return;
  }
}

void conectar_Servidor() {
  // Se intenta reconectar
  if(WiFi.status() == WL_CONNECTED){
    while (!client.connected()) {
      Serial.println("Intentando conectar con el broker MQTT...");
      // Se intenta la conexióno con el servidor
      char mensajeLastwill[60];
      snprintf(mensajeLastwill, 60, "{\"disp\":\"%s-%02X:%02X:%02X:%02X:%02X:%02X\",\"estado\":\"inactivo\"}",tipoSensor, MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5]);
      const char* topicoLastwill = "sistema/dispositivos/sensores";

      if (client.connect(IDCliente,topicoLastwill,0,false,mensajeLastwill)) {
        Serial.println("Dispositio conectado");
        //el dispositivo se conecta y envía el estado al sistma
        char jsonMensaje[60];
        snprintf(jsonMensaje, 60, "{\"disp\":\"%s-%02X:%02X:%02X:%02X:%02X:%02X\",\"estado\":\"activo\"}", tipoSensor, MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5]);

        client.publish("sistema/dispositivos/sensores",jsonMensaje);
        client.subscribe("sistema/dispositivos/estado");
      }
      if(WiFi.status() != WL_CONNECTED) break;
      delay(5000);
      digitalWrite(pinLed, LOW);
      delay(300);
      digitalWrite(pinLed, HIGH);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // configuramos el led de la placa como salida
  Serial.begin(115200);
  pinMode(pinLed, OUTPUT);
  pinMode(pinTx, OUTPUT);
  pinMode(pinLecturas, OUTPUT);

  digitalWrite(pinLed, HIGH);
  digitalWrite(pinTx, LOW);
  digitalWrite(pinLecturas, LOW);

  SPIFFS.begin();
  delay(100);
   
  // sensorFluido.begin();
  // if(!bme.begin(0x76)){
  //   Serial.println("Error al iniciar el sensor BME, revisar la conexion");
  //   while(1);
  // }

  estadoWiFi = conectar_WiFi();

  if(estadoWiFi == 1){
    client.setServer(mqtt_server, mqtt_port);
    client.setCallback(callback);
    WiFi.macAddress(MAC_WIFI);
    Serial.print("MAC: ");
    Serial.println(WiFi.macAddress());
    Serial.print("Ip: ");
    Serial.println(WiFi.localIP());
    Serial.print("Server: ");
    Serial.println(mqtt_server);
  }else{
    conectar_AP();
    Serial.print("Ip - AP: ");
    Serial.println(ipEstatica);
  }
  WiFi.printDiag(Serial);
  

  server.on("/",[](){server.send_P(200,"text/html", pagina_web);});
  server.on("/config", HTTP_POST, guardarDatoswifi);
  server.on("/reset",[](){
    server.send(200,"application/json","{\"estado\":\"reiniciando el dispositivo\"}");
    delay(1000);
    indicarOrden('r');
  });

  server.begin();
}


float calcularSensacion(float tempF, float humR){
   float temperaturaF = tempF;
   float humedadR = humR;
   float senTermica = -42.379+2.04901523*temperaturaF+10.14333127*humedadR-0.22475541*temperaturaF*humedadR-0.00683783*temperaturaF*temperaturaF-0.05481717*humedadR*humedadR+0.00122874*temperaturaF*temperaturaF*humedadR+0.00085282*temperaturaF*humedadR*humedadR-0.00000199*temperaturaF*temperaturaF*humedadR*humedadR;
   return senTermica;
}

void loop() {
  unsigned long ahora = millis();

  if(estadoWiFi == 1 && WiFi.status() == WL_CONNECTED){
    if(!client.connected()){
      conectar_Servidor();
    }else{
      client.loop();
      if(ahora - ultimoParpadeo > 500){
        ultimoParpadeo = ahora;
        digitalWrite(pinLed, !digitalRead(pinLed));
      }
      if(ahora - ultimoMensaje > 5000){
        ultimoMensaje = ahora;
        digitalWrite(pinLed, !digitalRead(pinLed));
        digitalWrite(pinLecturas, HIGH);
        valor += 0.01;
        // sensorFluido.requestTemperatures();
        // int lecturasPh[10];
        // double valorTotalLecturas = 0;

        // for(int i=0; i<10; i++){
        //     lecturasPh[i] = analogRead(pinSenPh);
        //     delay(10);
        // }

        // // se ordenan las lecturas al sensor de Ph
        // for(int i=0; i<9; i++){
        //    for(int j=0; j<10; j++){
        //       if(lecturasPh[i]>lecturasPh[j]){
        //          int aux = lecturasPh[i];
        //          lecturasPh[i] = lecturasPh[j];
        //          lecturasPh[j] = aux;
        //       }
        //    }
        // }

        // for(int i=2; i<8; i++){valorTotalLecturas += lecturasPh[i];}
        // valorTotalLecturas = valorTotalLecturas/6;

        // float voltajeSensorPh = valorTotalLecturas*3.33/1024.0;
        // float valorPh = 3.5*voltajeSensorPh+OffsetPh; // calculando el ph mediante offset
        // //hacer la calibracióno de los valores de voltaje obtenidos con un voltaje de 3.3V en la alimentación del sensor
        // //float valorPh = -5.70 * voltajeSensorPh + 21.338; // calculando el ph mediante pendiente 
        // float temFluido = sensorFluido.getTempCByIndex(0);
        // float humAire = bme.humidityValue();
        // float temAireC = bme.temperatureValue();
        // float temAireF = temAireC*9.0/5.0+32;
        // float senAire = (calcularSensacion(temAireF, humAire)-32)*(5.0/9.0);
        // float puntoRocio = temAireC-((100-humAire)/5.0);
        // float presionAtm = bme.pressureValue() / 100.0F;
        // float altitud = bme.altitudeValue(PRESION_NIVEL_MAR);
        digitalWrite(pinLecturas, LOW);

        digitalWrite(pinTx, HIGH);      
        char jsonTemFluido[90];
        char jsonPhFluido[90];
        char jsonTemAire[90];
        char jsonHumAire[90];
        char jsonSensacion[90];
        char jsonPuntoRocio[90];
        char jsonPresionAtm[90];

        snprintf(jsonPhFluido, 90, "{\"disp\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"variable\":\"ph_fluido\",\"valor\":%.2f}", MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], valor);
        snprintf(jsonTemFluido, 90, "{\"disp\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"variable\":\"temp_fluido\",\"valor\":%.2f}", MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], valor);
        snprintf(jsonTemAire, 90, "{\"disp\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"variable\":\"temp_aire\",\"valor\":%.2f}", MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], valor);
        snprintf(jsonHumAire, 90, "{\"disp\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"variable\":\"humedad\",\"valor\":%.2f}", MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], valor);
        snprintf(jsonSensacion, 90, "{\"disp\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"variable\":\"sensacion\",\"valor\":%.2f}", MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], valor);
        snprintf(jsonPuntoRocio, 90, "{\"disp\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"variable\":\"punto_rocio\",\"valor\":%.2f}", MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], valor);
        snprintf(jsonPresionAtm, 90, "{\"disp\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"variable\":\"presion\",\"valor\":%.2f}", MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], valor);

        
        if(client.connected()){
          client.publish("sistema/phFluido/sensores", jsonPhFluido);
          client.publish("sistema/temFluido/sensores", jsonTemFluido);
          client.publish("sistema/temAire/sensores", jsonTemAire);
          client.publish("sistema/humAire/sensores", jsonHumAire);
          client.publish("sistema/senAire/sensores", jsonSensacion);
          client.publish("sistema/punRocio/sensores", jsonPuntoRocio);
          client.publish("sistema/presionAtm/sensores", jsonPresionAtm);
        }
        digitalWrite(pinTx, LOW);
      }
    }
  }else if(estadoWiFi == 1 && WiFi.status() != WL_CONNECTED){
    
    if(ahora - ultimoParpadeo > 100){
      
      digitalWrite(pinLed, !digitalRead(pinLed));
      if(contadorReconexionWiFi++ > 70){
      
        Serial.println("Dispositivo desconectado de la red WiFi");
        contadorReconexionWiFi = 0;
        conectar_WiFi();
      }
    }
  }else if(estadoWiFi == -1){
    //if(client.connected()){client.loop();}
  
    if(ahora - ultimoParpadeo > 100){
      ultimoParpadeo = ahora;
  
      digitalWrite(pinLed, !digitalRead(pinLed));
    }
  }
  server.handleClient();
}