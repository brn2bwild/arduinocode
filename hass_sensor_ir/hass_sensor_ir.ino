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

#define intervaloMensaje      5000
#define intervaloOK           500
#define intervaloNook         200
#define intervaloMostrardatos 5000

#define estadoON  "ON"
#define estadoOFF "OFF"

#if (SSD1306_LCDHEIGHT != 64)
#error("Cantidad de pixeles de altura incorrecta, verificar Adafruit_SSD1306.h");
#endif

#define OLED_RESET  LED_BUILTIN

const int mqttPort = 1883;   // cadena que sirve para que los dispositivos se comuniquen con el broker de manera individual
char mqttServer[50], topicoComando[50], topicoEstado[50], topicoDatosTemIR[60], topicoDatosTemAire[60], topicoDatosHumAire[60], topicoDatosTemSonda[60], topicoDatosSenTermica[60], topicoDatosPuntoRoc[60], topicoDatosPreAtm[60];
unsigned long tiempoAnterior = 0, tiempoMostrarDatos;
uint8_t contadorDatos = 0;

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
     <input class='input' id='mqttserver' placeholder="Server" maxlength="40">
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
Adafruit_SSD1306 display(OLED_RESET);         //se inicializa la instancia de la pantalla oled
Adafruit_MLX90614 mlx = Adafruit_MLX90614();  //se inicializa la instancia del sensor de temperatura infrarrojo
OneWire oneWire(pinSenFlu);                   //se inicializa la instancia del sensor de temperatura SD18B20
DallasTemperature sondaTemperatura(&oneWire);     
DFRobot_BME280 bme;                           //se inicializa la instancia del sensor bme280

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
          snprintf(topicoDatosTemIR, 60, "data/%s/temIR", _mqttid);
          snprintf(topicoDatosTemAire, 60, "data/%s/temAire", _mqttid);
          snprintf(topicoDatosHumAire, 60, "data/%s/humAire", _mqttid);
          snprintf(topicoDatosTemSonda, 60, "data/%s/temSonda", _mqttid);
          snprintf(topicoDatosSenTermica, 60, "data/%s/senTermica", _mqttid);
          snprintf(topicoDatosPuntoRoc, 60, "data/%s/punRocio", _mqttid);
          snprintf(topicoDatosPreAtm, 60, "data/%s/preAtm", _mqttid);
          //Serial.println(topicoDatosTemAire);

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

void setup() {
  Serial.begin(115200);
  Serial.println("\nConfigurando entradas y salidas...");
  pinMode(pinLed, OUTPUT);
  pinMode(pinBotonArriba, INPUT);
  pinMode(pinBotonAbajo, INPUT);

  Serial.println("Inicializando sensor de temperatura infrarrojo...");
  mlx.begin();  //éstas líneas se tienen que comentar para probar el dispositivo sin sensores
  delay(200);

  Serial.println("Inicializando sensor de humedad-temperatura...");
  if(!bme.begin(0x76)){ //éstas líneas se tienen que comentar para probar el dispositivo sin sensores
    Serial.println("Error al inicializar el sensor de humedad-temperatura..."); //éstas líneas se tienen que comentar para probar el dispositivo sin sensores
    while(1); //éstas líneas se tienen que comentar para probar el dispositivo sin sensores
  } //éstas líneas se tienen que comentar para probar el dispositivo sin sensores
  delay(200);

  Serial.println("Inicializando sonda de temperatura...");
  sondaTemperatura.begin(); //éstas líneas se tienen que comentar para probar el dispositivo sin sensores
  delay(200);

  Serial.println("Inicializando display...");
   display.begin(SSD1306_SWITCHCAPVCC, 0x3C); //éstas líneas se tienen que comentar para probar el dispositivo sin sensores
   display.clearDisplay(); //éstas líneas se tienen que comentar para probar el dispositivo sin sensores
  delay(200);
   display.setTextSize(2); //éstas líneas se tienen que comentar para probar el dispositivo sin sensores
   display.setTextColor(WHITE); //éstas líneas se tienen que comentar para probar el dispositivo sin sensores
   display.setCursor(0,0); //éstas líneas se tienen que comentar para probar el dispositivo sin sensores
   display.clearDisplay(); //éstas líneas se tienen que comentar para probar el dispositivo sin sensores
   display.println("Conectando"); //éstas líneas se tienen que comentar para probar el dispositivo sin sensores
   display.drawXBitmap(30, 20,  WiFi_Logo_bits, WiFi_Logo_width, WiFi_Logo_height, WHITE); //éstas líneas se tienen que comentar para probar el dispositivo sin sensores
   display.display(); //éstas líneas se tienen que comentar para probar el dispositivo sin sensores

  SPIFFS.begin();
  delay(200);

  estadoWiFi = conectar_WiFi();
  if(estadoWiFi){
    if(conectar_Servidor()){
      Serial.println("Dispositivo conectado con el servidor");
    }else{
      Serial.println("Error al conectar el dispositivo con el servidor");
    }
    Serial.print("Ip: ");
    Serial.println(WiFi.localIP());
  }else{
    conectar_AP();
    Serial.print("Ip - AP: ");
    Serial.println(ipEstatica);
  }
  Serial.print("MAC: ");
  Serial.println(mac);
  WiFi.printDiag(Serial);

  server.on("/",[](){server.send_P(200,"text/html", pagina_web);});
  server.on("/config", HTTP_POST, guardarDatoswifi);
  server.on("/reset",[](){
    server.send(200,"application/json","{\"estado\":\"reiniciando el dispositivo\"}");
    delay(1000);
    ESP.restart();
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
   sondaTemperatura.requestTemperatures(); //éstas líneas se tienen que comentar para probar el dispositivo sin sensores
   temSondaC = sondaTemperatura.getTempCByIndex(0); //éstas líneas se tienen que comentar para probar el dispositivo sin sensores
   temObjetoC = mlx.readObjectTempC(); //éstas líneas se tienen que comentar para probar el dispositivo sin sensores
   temAireC = (bme.temperatureValue()+mlx.readAmbientTempC())/2; //éstas líneas se tienen que comentar para probar el dispositivo sin sensores
   humAire = bme.humidityValue(); //éstas líneas se tienen que comentar para probar el dispositivo sin sensores
   temAireF = temAireC*9.0/5.0+32; //éstas líneas se tienen que comentar para probar el dispositivo sin sensores
   senAire = (calcularSensacion(temAireF, humAire)-32)*(5.0/9.0); //éstas líneas se tienen que comentar para probar el dispositivo sin sensores
   puntoRocioC = temAireC-((100-humAire)/5.0); //éstas líneas se tienen que comentar para probar el dispositivo sin sensores
   presionAtm = bme.pressureValue() / 100.0F; //éstas líneas se tienen que comentar para probar el dispositivo sin sensores
  float altitud = bme.altitudeValue(PRESION_NIVEL_MAR); //éstas líneas se tienen que comentar para probar el dispositivo sin sensores

  // temSondaC = random(100);
  // temObjetoC = random(100);
  // temAireC = random(100);
  // humAire = random(100);
  // temAireF = random(100);
  // senAire = random(100);
  // puntoRocioC = random(100);
  // presionAtm = random(100);
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
  leerDatos(); //éstas líneas se tienen que comentar para probar el dispositivo sin sensores
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
  leerDatos(); //éstas líneas se tienen que comentar para probar el dispositivo sin sensores
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
  leerDatos(); //estas líneas se tienen que comentar para probar el código sin los sensores
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
  leerDatos(); //estas líneas se tienen que comentar para probar el código sin los sensores
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
  leerDatos(); //estas líneas se tienen que comentar para probar el código sin los sensores
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
  leerDatos(); //estas líneas se tienen que comentar para probar el código sin los sensores
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
  leerDatos(); //estas líneas se tienen que comentar para probar el código sin los sensores
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
  leerDatos(); //estas líneas se tienen que comentar para probar el código sin los sensores
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

  if((unsigned long)(ahora - tiempoMostrarDatos) >= intervaloMostrardatos){
    tiempoMostrarDatos = ahora;
    display.clearDisplay();
    funciones[funcionActual]();    
    digitalWrite(pinLed, !digitalRead(pinLed));
  }

  if(estadoWiFi){
    if((unsigned long)(ahora - tiempoAnterior) > intervaloOK){
      tiempoAnterior = ahora;
      digitalWrite(pinLed, !digitalRead(pinLed));
      if(contadorDatos++ >= 9){
        contadorDatos = 0;
        if(!client.connected()){
          conectar_Servidor();
        }else{
          client.loop();

          leerDatos();
          char dato[8];
          if(habilitarDispositivo){
            leerDatos();
            display.setTextSize(2);
            display.setCursor(105,47);
            display.println("Tx");
            display.display();
            dtostrf(temObjetoC, 6, 2, dato);
            client.publish(topicoDatosTemIR, dato);
            dtostrf(temAireC, 6, 2, dato);
            client.publish(topicoDatosTemAire, dato);
            dtostrf(humAire, 6, 2, dato);
            client.publish(topicoDatosHumAire, dato);
            dtostrf(temSondaC, 6, 2, dato);
            client.publish(topicoDatosTemSonda, dato);
            dtostrf(senAire, 6, 2, dato);
            client.publish(topicoDatosSenTermica, dato);
            dtostrf(puntoRocioC, 6, 2, dato);
            client.publish(topicoDatosPuntoRoc, dato);
            dtostrf(presionAtm, 6, 2, dato);
            client.publish(topicoDatosPreAtm, dato);
            Serial.println("Datos enviados");
          }
        }
      }
      if(WiFi.status() != WL_CONNECTED){
        Serial.println("Dispositivo desconectado de la red WiFi\nIntentando reconectar...");
        conectar_WiFi();
      }
    }
  }else{
    if((unsigned long)(ahora - tiempoAnterior) > intervaloNook){
      tiempoAnterior = ahora;
      digitalWrite(pinLed, !digitalRead(pinLed));
    }
  }
  server.handleClient();
}