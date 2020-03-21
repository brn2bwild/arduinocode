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
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <Adafruit_MLX90614.h>
//#include "SSD1306Wire.h"

#include "images.h"

#define PRESION_NIVEL_MAR     1013.25f
#define BME_CS                10

#define pinLed            2
#define pinBotonArriba    14 // d5
#define pinBotonAbajo     12 // d6
#define pinSenFlu         0  // d3

#if (SSD1306_LCDHEIGHT != 64)
#error("Cantidad de pixeles de altura incorrecta, verificar Adafruit_SSD1306.h");
#endif

#define OLED_RESET  LED_BUILTIN

char mqttServer[30];  //se debe configurar la ip del raspberry para que quede estática
//const char* mqttServer = "iot.eclipse.org";  //se debe configurar la ip del raspberry para que quede estática
const int mqttPort = 1883;
char idCliente[20];   // cadena que sirve para que los dispositivos se comuniquen con el broker de manera individual
//const char* idCliente = "sT0100001";     // cadena que sirve para que los dispositivos se comuniquen con el broker de manera individual
const char* tipoSensor = "sensor01";

const int intervaloMensaje = 5000;
const int intMostrarDatos = 500;
const int intParpadeoNoOk = 200;

unsigned long tiempoMensajeAnterior = 0; //varible que controlará el tiempo de envío de datos
unsigned long tiempoParpadeoAnterior = 0;
unsigned long tiempoMostrarDatos = 0;

float temSondaC;
float temObjetoC;
float temAireC;
float humAire;
float temAireF;
float senAire;
float puntoRocioC;
float presionAtm;

uint8_t MAC_AP[WL_MAC_ADDR_LENGTH];// variable que guardará la MAC_AP del dispositivo
uint8_t MAC_WIFI[6];
int estadoWiFi;
bool habilitarDispositivo = true;

IPAddress ipEstatica(1,1,1,1);
IPAddress ipMascara(255,255,255,0);

char pagina_web[] PROGMEM = R"=====(
<!DOCTYPE HTML>
<html>
<head>
<style>
     html,body{
          margin:auto;
          text-align:center;
          font-family:sans-serif;
     }
     .form{
          padding:10px;
          margin:auto;
          background-color:#f05f40; 
          width:80%;
          border-radius:20px;
          text-align:center;
     }
     h1{
          color:#F05F40;
          font-size:70px;
          margin: 2% auto;
     }
     .input{
          width:85%;
          font-size:35px;
          margin:2% auto 1.5% auto;
          border:2.5px solid #FFF;
          background-color:transparent;
          color:#FFFFFF;
          border-radius:20px;
          text-align: center;
     }
     ::-webkit-input-placeholder{
          color:rgba(255,255,255,0.9);
     }
     .boton{
          color:#F05F40;
          background-color:#FFFFFF;
          font-weight:600;
          font-size:2.5rem;
          border:none;
          width:80%;
          cursor:pointer;
          border-radius:25px;
          margin:1.5% auto;
     }
</style>
<meta name='viewport' content='width=device-width, initial-scale=1.0' charset='utf-8'>
<title>Dispositivo IoT</title>
</head>
<body>
     <h1>Sensor IoT</h1>
     <div class='form'>
          <input class='input' id='ssid' placeholder="Red WiFi">
          <input type="password" class='input' id='pass' placeholder="Contraseña WiFi">
          <input class='input' id='server' placeholder="Server" maxlength="30">
          <input class='input' id='id' placeholder="ID Dispositivo" maxlength="20">
          <button class='boton' onclick="guardar()">GUARDAR</button>
     </div>
</body>
</form>
<script type="text/javascript">
     function guardar(){
          var ssid = document.getElementById('ssid').value;
          var pass = document.getElementById('pass').value;
          var server = document.getElementById('server').value;
          var id = document.getElementById('id').value;
          var data = {ssid:ssid, pass:pass, server:server, id:id};

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
Adafruit_SSD1306 display(OLED_RESET);         //se inicializa la instancia de la pantalla oled
Adafruit_MLX90614 mlx = Adafruit_MLX90614();  //se inicializa la instancia del sensor de temperatura infrarrojo
OneWire oneWire(pinSenFlu);                   //se inicializa la instancia del sensor de temperatura SD18B20
DallasTemperature sondaTemperatura(&oneWire);     
DFRobot_BME280 bme;                           //se inicializa la instancia del sensor bme280

void indicarOrden(char orden){
  char jsonMensaje[150];
  if(orden == 'r'){
    snprintf(jsonMensaje, 150, "{\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"id\":\"%s\",\"tipo\":\"%s\",\"estado\":\"reiniciado\"}", MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], idCliente, tipoSensor);
    client.publish("sistema/dispositivos/sensores",jsonMensaje);
    delay(500);
    ESP.restart();
  }else if(orden == 'd'){
    habilitarDispositivo = false;
    snprintf(jsonMensaje, 150, "{\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"id\":\"%s\",\"tipo\":\"%s\",\"estado\":\"deshabilitado\"}", MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], idCliente, tipoSensor);
  }else if(orden == 'h'){
    habilitarDispositivo = true;
    snprintf(jsonMensaje, 150, "{\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"id\":\"%s\",\"tipo\":\"%s\",\"estado\":\"activo\"}", MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], idCliente, tipoSensor);
  }
  client.publish("sistema/dispositivos/sensores",jsonMensaje);
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

int conectar_WiFi(){
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
    Serial.println("");
    return 1;
  }else{
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
  //char cadenaReinicio[100];
  //memset(cadenaReinicio, 0, sizeof(cadenaReinicio));
  char mac[20];
  snprintf(mac, 20, "%02X:%02X:%02X:%02X:%02X:%02X", MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5]);

  Serial.print("Mensaje recibido [");Serial.print(topic);Serial.println("]");

  //for (int i = 0; i < length; i++) {cadenaReinicio[i] = (char)payload[i];}

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(payload);

  if(!json.success()){Serial.println("Error al recibir comando: ruta de control");}
  const char* macOrden = json["mac"];
  const char* orden = json["orden"];

  if(sizeof(macOrden) > 0){
    int igualID = strcmp(macOrden, mac);

    if(igualID == 0){
      Serial.println(macOrden);
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
    if (!client.connected()) {
      Serial.println("Intentando conectar con el broker MQTT...");
      // Se intenta la conexióno con el servidor
      char mensajeLastwill[150];
      char mac[20];
      snprintf(mensajeLastwill, 150, "{\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"id\":\"%s\",\"tipo\":\"%s\",\"estado\":\"inactivo\"}", MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], idCliente, tipoSensor);
      snprintf(mac, 20, "%02X:%02X:%02X:%02X:%02X:%02X", MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5]);
      const char* topicoLastwill = "sistema/dispositivos/sensores";

      if(client.connect(mac,topicoLastwill,0,false,mensajeLastwill)) {
        Serial.println("Dispositio conectado");
        //el dispositivo se conecta y envía el estado al sistma
        char jsonMensaje[150];
        snprintf(jsonMensaje, 150, "{\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"id\":\"%s\",\"tipo\":\"%s\",\"estado\":\"activo\"}", MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], idCliente, tipoSensor);
        client.publish("sistema/dispositivos/sensores",jsonMensaje);
        client.subscribe("sistema/dispositivos/estado");
      }else{
        Serial.println("Dispositivo no se pudo conectar con el servidor...");
        delay(100);
      }
    }else{
      Serial.println("Dispositivo conectado con el servidor...");
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nConfigurando entradas y salidas...");
  pinMode(pinLed, OUTPUT);
  pinMode(pinBotonArriba, INPUT);
  pinMode(pinBotonAbajo, INPUT);

  Serial.println("Inicializando sensor de temperatura infrarrojo...");
  //mlx.begin();
  delay(200);

  Serial.println("Inicializando sensor de humedad-temperatura...");
  //if(!bme.begin(0x76)){
  //  Serial.println("Error al inicializar el sensor de humedad-temperatura...");
  //  while(1);
  //}
  delay(200);

  Serial.println("Inicializando sonda de temperatura...");
  //sondaTemperatura.begin();
  delay(200);

  Serial.println("Inicializando display...");
  // display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  // display.clearDisplay();
  delay(200);
  // display.setTextSize(2);
  // display.setTextColor(WHITE);
  // display.setCursor(0,0);
  // display.clearDisplay();
  // display.println("Conectando");
  // display.drawXBitmap(30, 20,  WiFi_Logo_bits, WiFi_Logo_width, WiFi_Logo_height, WHITE);
  // display.display();

  SPIFFS.begin();
  delay(200);

  estadoWiFi = conectar_WiFi();

  if(estadoWiFi == 1){
    if(SPIFFS.exists("/config.json")){
      const char *_mqttServer = "", *_idCliente = "";
      File configFile = SPIFFS.open("/config.json","r");
      if(configFile){
        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        configFile.close();

        DynamicJsonBuffer jsonBuffer;
        JsonObject &jObject = jsonBuffer.parseObject(buf.get());
        if(jObject.success()){

          _mqttServer = jObject["server"];
          _idCliente = jObject["id"];

          snprintf(mqttServer, 30, "%s", _mqttServer);
          snprintf(idCliente, 20, "%s", _idCliente);
        }
      }
    }
    client.setServer(mqttServer, mqttPort);
    client.setCallback(callback);
    WiFi.macAddress(MAC_WIFI);
    Serial.print("MAC: ");
    Serial.println(WiFi.macAddress());
    Serial.print("Ip: ");
    Serial.println(WiFi.localIP());
    Serial.print("Server: ");
    Serial.println(mqttServer);
    Serial.print("ID: ");
    Serial.println(idCliente);
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

void leerDatos(){
  // sondaTemperatura.requestTemperatures();
  // temSondaC = sondaTemperatura.getTempCByIndex(0);
  // temObjetoC = mlx.readObjectTempC();
  // temAireC = (bme.temperatureValue()+mlx.readAmbientTempC())/2;
  // humAire = bme.humidityValue();
  // temAireF = temAireC*9.0/5.0+32;
  // senAire = (calcularSensacion(temAireF, humAire)-32)*(5.0/9.0);
  // puntoRocioC = temAireC-((100-humAire)/5.0);
  // presionAtm = bme.pressureValue() / 100.0F;
  //float altitud = bme.altitudeValue(PRESION_NIVEL_MAR);

  temSondaC = random(100);
  temObjetoC = random(100);
  temAireC = random(100);
  humAire = random(100);
  temAireF = random(100);
  senAire = random(100);
  puntoRocioC = random(100);
  presionAtm = random(100);
}

void mostrarInfo(){
  if(WiFi.status() != WL_CONNECTED){
    display.setTextSize(2);
    display.setCursor(35,47);
    display.println("nW");
  }else{
    display.setTextSize(2);
    display.setCursor(35,47);
    display.println("W");
  }
  if(!client.connected()){
    display.setTextSize(2);
    display.setCursor(70,47);
    display.println("nS");
  }else{
    display.setTextSize(2);
    display.setCursor(70,47);
    display.println("S");
  }
}

void humedadAire(void) {
  //leerDatos();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.println("Hum. Aire:");
  display.setTextSize(3);
  display.setCursor(5,20);
  display.print(bme.humidityValue(),1);
  display.print((char)247);
  display.println("%");
  mostrarInfo();
  display.display();
}

void temperaturaAire(void) {
  //leerDatos();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.println("Tem. Aire:");
  display.setTextSize(3);
  display.setCursor(5,20);
  //display.print(bme.temperatureValue(),1); //se toma la temperatura el aire con el sensor bme280
  //display.print(mlx.readAmbientTempC(),1); // se toma la temperatura del aire con el sensor mlx90614
  display.print(((float)bme.temperatureValue()+(float)mlx.readAmbientTempC())/2,1);
  display.print((char)247);
  display.println("C");
  mostrarInfo();
  display.display();
  //Serial.println(bme.temperatureValue());
  //Serial.println(mlx.readAmbientTempC());
}

void temperaturaObjeto(void) {
  //leerDatos();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.println("Tem. Obj:");
  display.setTextSize(3);
  display.setCursor(5,20);
  display.print(mlx.readObjectTempC(),1);
  display.print((char)247);
  display.println("C");
  mostrarInfo();
  display.display();
}

void temperaturaSonda(void) {
  //leerDatos();
  sondaTemperatura.requestTemperatures();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.println("Tem. Snd:");
  display.setTextSize(3);
  display.setCursor(5,20);
  display.print(sondaTemperatura.getTempCByIndex(0),1);
  display.print((char)247);
  display.println("C");
  mostrarInfo();
  display.display();
}

void sensacionTermica(void) {
  //leerDatos();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.println("Sen. Ter:");
  display.setTextSize(3);
  display.setCursor(5,20);
  display.print((calcularSensacion((bme.temperatureValue()*9.0/5.0+32), bme.humidityValue())-32)*(5.0/9.0),1);
  display.print((char)247);
  display.println("C");
  mostrarInfo();
  display.display();
}

void puntoRocio(void) {
  //leerDatos();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.println("Pun. Roc:");
  display.setTextSize(3);
  display.setCursor(5,20);
  display.print((bme.temperatureValue()-((100-bme.humidityValue())/5.0)),1);
  display.print((char)247);
  display.println("C");
  mostrarInfo();
  display.display();
}

void presionAtmosferica(void) {
  //leerDatos();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.println("Pre. Atm:");
  display.setTextSize(2);
  display.setCursor(5,20);
  display.print((bme.pressureValue() / 100.0F),1);
  display.println(" hPa");
  mostrarInfo();
  display.display();
}

void infoWiFi(void) {
  //leerDatos();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.println("MAC: ");
  display.println(WiFi.macAddress());
  if(WiFi.status() == WL_CONNECTED){
    display.println("IP: ");
    display.println(WiFi.localIP());
  } else{
    display.println("IP: ");
    display.println(ipEstatica);
  }
  display.println("SERVER: ");
  display.println(mqttServer);
  display.display();
}

typedef void (*Funcion)(void);
Funcion funciones[] = {temperaturaObjeto, temperaturaAire, humedadAire, temperaturaSonda, sensacionTermica, puntoRocio, presionAtmosferica, infoWiFi};
int cantidadFunciones = (sizeof(funciones)/sizeof(Funcion));
int funcionActual = 0;

void loop() {
  unsigned long ahora = millis();

  if(!digitalRead(pinBotonArriba)){
    delay(350);
    if(funcionActual++ >= 6) funcionActual = 0;
  }  
  if(!digitalRead(pinBotonAbajo)){
    delay(350);
    funcionActual = 7;
  }

  if((unsigned long)(ahora - tiempoMostrarDatos) >= intMostrarDatos){
    tiempoMostrarDatos = ahora;
    //display.clearDisplay();
    //funciones[funcionActual]();    
    digitalWrite(pinLed, !digitalRead(pinLed));
  }

  if((unsigned long)(ahora - tiempoMensajeAnterior) >= intervaloMensaje){
    tiempoMensajeAnterior = ahora;
    if((estadoWiFi == 1) && (WiFi.status() == WL_CONNECTED)){
      if(!client.connected()){
        conectar_Servidor();
      }else{
        client.loop();

        leerDatos();

        char jsonMensaje[180];

        snprintf(jsonMensaje, 180, "{\"mac\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"id\":\"%s\",\"tS\":%.1f,\"tO\":%.1f,\"tA\":%.1f,\"hA\":%.1f,\"sT\":%.1f,\"pR\":%.1f,\"pA\":%.1f}", MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], idCliente, temSondaC, temObjetoC, temAireC, humAire, senAire, puntoRocioC, presionAtm);

        if(habilitarDispositivo){
          Serial.println(jsonMensaje);
          display.setTextSize(2);
          display.setCursor(105,47);
          display.println("Tx");
          display.display();
          if(!client.publish("sistema/sensores", jsonMensaje)) Serial.println("Error al enviar el mensaje");
        }
      }
    }else if((estadoWiFi == 1) && (WiFi.status() != WL_CONNECTED)){ 
      Serial.println("Dispositivo desconectado de la red WiFi\nIntentando reconectar...");
      conectar_WiFi();
    }
  }
  if((estadoWiFi == -1) && ((unsigned long)(ahora - tiempoParpadeoAnterior) >= intParpadeoNoOk)){
    tiempoParpadeoAnterior = ahora;
    digitalWrite(pinLed, !digitalRead(pinLed));
  }
  server.handleClient();
}