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

#define PRESION_NIVEL_MAR     1013.25f
#define BME_CS                10

#define pinLed            2
#define pinBotonArriba    14 // d5
#define pinBotonAbajo     12 // d6
#define pinSenFlu         0  // d3
//#define pinSenPh          A0 

//#define OffsetPh        0.0

#if (SSD1306_LCDHEIGHT != 64)
#error("Cantidad de pixeles de altura incorrecta, verificar Adafruit_SSD1306.h");
#endif

#define OLED_RESET  LED_BUILTIN

const char* mqtt_server = "192.168.10.200";  //se debe configurar la ip del raspberry para que quede estática
//const char* mqtt_server = "iot.eclipse.org";  //se debe configurar la ip del raspberry para que quede estática
const int mqtt_port = 1883;
const char* IDCliente = "keYJw21dQ3g=";   // cadena: stA00001 codificada con algoritmo arcfour en modo CBC clave: sensor
//const char* IDCliente = "keYJw21dQ3s=";     // cadena: sTA00002 codificada con algoritmo arcfour en modo CBC clave: sensor
const char* tipoSensor = "sensor01";

const int intervaloMensaje = 10000;
const int intMostrarDatos = 500;
const int intParpadeoNoOk = 200;

//variables utilizadas para conexión con el servidor MQTT
//char cadenaTopico[100]; // variable que contendrá la ruta del topico al cual se subscribirá el dispositivo
//char mensaje[50];

unsigned long tiempoMensajeAnterior = 0; //varible que controlará el tiempo de envío de datos
unsigned long tiempoParpadeoAnterior = 0;
unsigned long tiempoMostrarDatos = 0;
unsigned char contadorReconexionWiFi = 0;

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
Adafruit_SSD1306 display(OLED_RESET);         //se inicializa la instancia de la pantalla oled
Adafruit_MLX90614 mlx = Adafruit_MLX90614();  //se inicializa la instancia del sensor de temperatura infrarrojo
OneWire oneWire(pinSenFlu);                   //se inicializa la instancia del sensor de temperatura SD18B20
DallasTemperature sondaTemperatura(&oneWire);     
DFRobot_BME280 bme;                           //se inicializa la instancia del sensor bme280

void indicarOrden(char orden){
  char jsonMensaje[80];
  if(orden == 'r'){
    snprintf(jsonMensaje, 80, "{\"disp\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"tipo\":\"%s\",\"estado\":\"reiniciado\"}", MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], tipoSensor);
    client.publish("sistema/dispositivos/sensores",jsonMensaje);
    delay(1000);
    ESP.restart();
  }else if(orden == 'd'){
    snprintf(jsonMensaje, 80, "{\"disp\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"tipo\":\"%s\",\"estado\":\"deshabilitado\"}", MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], tipoSensor);
  }else if(orden == 'h'){
    snprintf(jsonMensaje, 80, "{\"disp\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"tipo\":\"%s\",\"estado\":\"activo\"}", MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], tipoSensor);
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

        Serial.print("\nIntentado conectar a red WiFi: "); Serial.println(_ssid);
        unsigned long tiempoInicio = millis();
        while(WiFi.status() != WL_CONNECTED){
          delay(1000);
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
  char cadenaReinicio[100];
  memset(cadenaReinicio, 0, sizeof(cadenaReinicio));

  Serial.print("Mensaje recibido [");Serial.print(topic);Serial.println("]");

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
    if (!client.connected()) {
      Serial.println("Intentando conectar con el broker MQTT...");
      // Se intenta la conexióno con el servidor
      char mensajeLastwill[80];
      snprintf(mensajeLastwill, 80, "{\"disp\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"tipo\":\"%s\",\"estado\":\"inactivo\"}", MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5],tipoSensor);
      const char* topicoLastwill = "sistema/dispositivos/sensores";

      if(client.connect(IDCliente,topicoLastwill,0,false,mensajeLastwill)) {
        Serial.println("Dispositio conectado");
        //el dispositivo se conecta y envía el estado al sistma
        char jsonMensaje[80];
        snprintf(jsonMensaje, 80, "{\"disp\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"tipo\":\"%s\",\"estado\":\"activo\"}", MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5],tipoSensor);
        client.publish("sistema/dispositivos/sensores",jsonMensaje);
        client.subscribe("sistema/dispositivos/estado");
      }else{
        Serial.println("Dispositivo no se pudo conectar con el servidor...");
      }
    }else{
      Serial.println("Dispositivo conectado con el servidor...");
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nConfigurando las entradas y salidas...");
  pinMode(pinLed, OUTPUT);
  pinMode(pinBotonArriba, INPUT);
  pinMode(pinBotonAbajo, INPUT);

  Serial.println("Inicializando el sensor de temperatura infrarrojo...");
  mlx.begin();
  delay(500);

  Serial.println("Inicializando el sensor de humedad-temperatura...");
  if(!bme.begin(0x76)){
    Serial.println("Error al inicializar el sensor de humedad-temperatura...");
    while(1);
  }
  delay(500);

  Serial.println("Inicializando la sonda de temperatura...");
  sondaTemperatura.begin();
  delay(500);

  Serial.println("Inicializando display...");
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  delay(1000);

  SPIFFS.begin();
  delay(100);

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


void leerDatos(){
  sondaTemperatura.requestTemperatures();
  temSondaC = sondaTemperatura.getTempCByIndex(0);
  temObjetoC = mlx.readObjectTempC();
  temAireC = (bme.temperatureValue()+mlx.readAmbientTempC())/2;
  humAire = bme.humidityValue();
  temAireF = temAireC*9.0/5.0+32;
  senAire = (calcularSensacion(temAireF, humAire)-32)*(5.0/9.0);
  puntoRocioC = temAireC-((100-humAire)/5.0);
  presionAtm = bme.pressureValue() / 100.0F;
  //float altitud = bme.altitudeValue(PRESION_NIVEL_MAR);

  // Serial.print("Ambient = "); Serial.print(mlx.readAmbientTempC()); 
  // Serial.print("*C\tObject = "); Serial.print(mlx.readObjectTempC());Serial.println("*C");
  // Serial.print("Temperatura = "); Serial.print(bme.temperatureValue());
  // Serial.print("*C\tHumedad = "); Serial.print(bme.humidityValue());Serial.println("%");
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
  display.display();
  Serial.println(bme.temperatureValue());
  Serial.println(mlx.readAmbientTempC());
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
  display.display();
}

void presionAtmosferica(void) {
  //leerDatos();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.println("Pre. Atm:");
  display.setTextSize(3);
  display.setCursor(5,20);
  display.println((bme.pressureValue() / 100.0F),1);
  display.println("hPa");
  display.display();
}

typedef void (*Funcion)(void);
Funcion funciones[] = {temperaturaObjeto, temperaturaAire, humedadAire, temperaturaSonda, sensacionTermica, puntoRocio, presionAtmosferica};
int cantidadFunciones = (sizeof(funciones)/sizeof(Funcion));
int funcionActual = 0;

void loop() {
  unsigned long ahora = millis();

  if(!digitalRead(pinBotonArriba)){
    delay(250);
    funcionActual = (funcionActual + 1) % cantidadFunciones;
  }  

  if((unsigned long)(ahora - tiempoMostrarDatos) >= intMostrarDatos){
    tiempoMostrarDatos = ahora;
    display.clearDisplay();
    funciones[funcionActual]();    
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

        char jsonTemSonda[90];
        char jsonTemObjeto[90];
        char jsonTemAire[90];
        char jsonHumAire[90];
        char jsonSensacion[90];
        char jsonPuntoRocio[90];
        char jsonPresionAtm[90];

        snprintf(jsonTemSonda, 90, "{\"disp\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"variable\":\"temp_sonda\",\"valor\":%.2f}", MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], temSondaC);
        snprintf(jsonTemObjeto, 90, "{\"disp\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"variable\":\"temp_objeto\",\"valor\":%.2f}", MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], temObjetoC);
        snprintf(jsonTemAire, 90, "{\"disp\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"variable\":\"temp_aire\",\"valor\":%.2f}", MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], temAireC);
        snprintf(jsonHumAire, 90, "{\"disp\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"variable\":\"humedad\",\"valor\":%.2f}", MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], humAire);
        snprintf(jsonSensacion, 90, "{\"disp\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"variable\":\"sensacion\",\"valor\":%.2f}", MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], senAire);
        snprintf(jsonPuntoRocio, 90, "{\"disp\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"variable\":\"punto_rocio\",\"valor\":%.2f}", MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], puntoRocioC);
        snprintf(jsonPresionAtm, 90, "{\"disp\":\"%02X:%02X:%02X:%02X:%02X:%02X\",\"variable\":\"presion\",\"valor\":%.2f}", MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], presionAtm);

        Serial.println("Enviando datos...");
        if(client.connected()){
          client.publish("sistema/temSonda/sensores", jsonTemSonda);
          client.publish("sistema/temObjeto/sensores", jsonTemObjeto);
          client.publish("sistema/temAire/sensores", jsonTemAire);
          client.publish("sistema/humAire/sensores", jsonHumAire);
          client.publish("sistema/senAire/sensores", jsonSensacion);
          client.publish("sistema/punRocio/sensores", jsonPuntoRocio);
          client.publish("sistema/presionAtm/sensores", jsonPresionAtm);
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