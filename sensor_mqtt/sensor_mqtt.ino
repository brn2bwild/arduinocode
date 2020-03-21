/*
 Basic ESP8266 MQTT example

 This sketch demonstrates the capabilities of the pubsub library in combination
 with the ESP8266 board/library.

 It connects to an MQTT server then:
  - publishes "hello world" to the topic "outTopic" every two seconds
  - subscribes to the topic "inTopic", printing out any messages
    it receives. NB - it assumes the received payloads are strings not binary
  - If the first character of the topic "inTopic" is an 1, switch ON the ESP Led,
    else switch it off

 It will reconnect to the server if the connection is lost using a blocking
 reconnect function. See the 'mqtt_reconnect_nonblocking' example for how to
 achieve the same result without blocking the main loop.

 To install the ESP8266 board, (using Arduino 1.6.4+):
  - Add the following 3rd party board manager under "File -> Preferences -> Additional Boards Manager URLs":
       http://arduino.esp8266.com/stable/package_esp8266com_index.json
  - Open the "Tools -> Board -> Board Manager" and click install for the ESP8266"
  - Select your ESP8266 in "Tools -> Board"

*/

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <math.h>
#include <time.h>
#include <DFRobot_BME280.h>

// Update these with values suitable for your network.

#define PRESION_NIVEL_MAR     1013.25f
#define BME_CS                10

#define pinLed          2
#define pinTx           14 // d5
#define pinLecturas     12 // d6
#define pinSenFlu       0  // d3
#define pinSenPh        A0 

#define OffsetPh        0.0

#define modoAP          0
#define modoWiFi        1

const char* ssid = "Tesalia-AP";
const char* password = "aahooraa";
const char* mqtt_server = "192.168.10.5";

const char* IDcliente = "keYJw21dQ3g=";   // cadena: stA00001 codificada con algoritmo arcfour en modo CBC clave: sensor
//const char* IDcliente = "keYJw21dQ3s=";     // cadena: sTA00002 codificada con algoritmo arcfour en modo CBC clave: sensor

//variables utilizadas para conexión con el servidor MQTT
char cadenaTopico[100]; // variable que contendrá la ruta del topico al cual se subscribirá el dispositivo
char mensaje[50];

//char tipoDispositivo[20] = "sensor_temperatura"

char ubicacionDispositivo[20] = "alebines";
char identificadorDispositivo[20] = "estanque";
//char identificadorDispositivo[20] = "cisterna";

long ultimoMensaje = 0; //varible que controlará el tiempo de envío de datos
long ultimoParpadeo = 0;
float valor = 0.0;

//String  eeprom_wifissid = ""; //variables que guardarán las credenciales WiFi
//String  eeprom_wifipass = "";
char eeprom_ssid[40];
char eeprom_pass[40];
uint8_t mac[WL_MAC_ADDR_LENGTH];// variable que guardará la MAC del dispositivo

String contenido = "";
int codStatus = 0;
bool dispositivoConfigurado = false; //variable  que controlará que el dispositivo esté configurado

//se inicializan las instancias que se encargarán de las conexiones WiFi, MQTT y el server AP
WiFiClient espClient;
PubSubClient client(espClient);
ESP8266WebServer server;

//se inicializan las instancias que se encargarán de los sensores
OneWire oneWire(pinSenFlu);
DallasTemperature sensorFluido(&oneWire);
DFRobot_BME280 bme;

void leer_EEPROM_char(){
  memset(eeprom_ssid, 0, sizeof(eeprom_ssid));
  for(int i = 0 ; i < 40 ; i++){ 
    if(EEPROM.read(i) != 0){
      eeprom_ssid[i] = (char)EEPROM.read(i);
    }
  }
  Serial.print("SSID: ");
  Serial.println(eeprom_ssid);

  memset(eeprom_pass, 0, sizeof(eeprom_pass));
  for(int i = 40 ; i < 80 ; i++){ 
    if(EEPROM.read(i) != 0){
      eeprom_pass[i-40] = (char)EEPROM.read(i);
    }
  }
  Serial.print("PASS: ");
  Serial.println(eeprom_pass);
}

void cargarServer_char(int tipoServer){
   if(tipoServer == modoAP){
      server.on("/", [](){
         contenido += "<!DOCTYPE HTML><html><form method='get' action='config'><head><style>html,body{";
         contenido += "width: 100%; height: 100%; margin: 0;  font-family: sans-serif;  } .form{  padding: 30px;  margin: 0 auto; transform: ";
         contenido += "translateY(15%);  border-radius: 4px; max-width: 500px; background-color: #f05f40;  } .input{ width:100%; font-size:30";
         contenido += "px; margin: 0 0 15px 0; border: 1.5px solid #FFF; background-color: transparent;  color: #FFFFFF; border-radius: 5px;";
         contenido += "  } .label{ color: #ffffff; font-size: 30px;  margin: 0 0 5px 0;  height: 300px } .boton{ color: #F05F40; ";
         contenido += "background-color: #FFFFFF;  font-weight: 600; font-size: 3rem;  border: none; width: 100%;  cursor: pointer;  ";
         contenido += "border-radius: 5px; } h1{ text-align: left; color: #F05F40; margin: 0 0 -50px 0;  padding: 30px 0 0 60px; font-size: ";
         contenido += "100px;  } h2{ text-align: center; color: #FFFFFF; margin: 0 auto; } span{ float: right; padding: 0 10% 0 0; font-size: 30px; }</style>  <meta name='viewport' content='";
         contenido += "width=device-width,initial-scale=1'><meta charset='utf-8'>  <title>IOT Device</title> </head> <body>  <h1>EBENTI-IOT</h1";
         contenido += "> <div class='form'>  <h2>Introduzca las credenciales</h2>  <label class='label'>SSID:</label><input class='input' name";
         contenido += "='ssid' maxlength=40> <label class='label'>PASS:</label><input class='input' name='pass' maxlength=40><button class='boton' type='submit'>GUARDAR</button";
         contenido += "> </div>  </body> </form> </html>";
         server.send(200,"text/html",contenido);
      });
      server.on("/config", [](){
         char querySSID[40]; 
         char queryPASS[40];
         memset(querySSID, 0, sizeof(querySSID));
         memset(queryPASS, 0, sizeof(queryPASS));
         server.arg("ssid").toCharArray(querySSID,sizeof(querySSID));
         server.arg("pass").toCharArray(queryPASS,sizeof(queryPASS));

         if(sizeof(querySSID) > 0 && sizeof(queryPASS) > 0){
            Serial.println("Limpiando EEPROM");
            for(int i = 0; i < 80; i++){EEPROM.write(i,0);}
            Serial.print("SSID: ");
            Serial.println(querySSID);
            Serial.print("PASS: ");
            Serial.println(queryPASS);

            for(int i = 0; i < sizeof(querySSID); i++){
               EEPROM.write(i, querySSID[i]);
               Serial.print(querySSID[i]);
            }
            Serial.print(",");
            for(int i = 0; i < sizeof(queryPASS); i++){
               EEPROM.write(40+i, queryPASS[i]);
               Serial.print(queryPASS[i]);
            }
            Serial.println();

            EEPROM.commit();

            contenido = "{\"Success\":\"datos guardados en la EEPROM, reinicie manualmente el dispositivo\"}";
            codStatus = 200;
         }else{
            contenido = "{\"Error:\":\"error al cargar los datos a la EEPROM\"}";
            codStatus = 404;
         }
         server.send(codStatus,"application/json",contenido);
      });
   }else if(tipoServer == modoWiFi){
      server.on("/",[](){
         server.send(200,"application/json","{\"success:\":\"dispositivo conectado\"}");
         Serial.println("Cliente conectado al dispositivo");
      });
      server.on("/reset_disp",[](){
         server.send(200,"application/json","{\"success:\":\"dispositivo reiniciado\"}");
         Serial.println("Reiniciando dispositivo");
         char jsonMensaje[50];
         snprintf(jsonMensaje, 50, "{\"dispositivo\":\"%s\",\"estado\":\"reiniciado\"}", identificadorDispositivo);
         client.publish("sistema/dispositivos/sensores",jsonMensaje);
         delay(1000);
         ESP.restart();
         // HTTPClient http;
         // http.begin(uri_reinicio);
         // http.addHeader("Authorization",token);
         // http.GET();
         // http.end();
         // delay(10);
         // ESP.restart();
      });
      server.on("/hardreset_disp",[](){
         server.send(200,"application/json","{\"success:\":\"dispositivo reseteado\"}");
         for(int i=0; i<80; i++){EEPROM.write(i,0);}
         EEPROM.commit();
         delay(1000);
         Serial.println("EEPROM borrada");
         WiFi.disconnect();
         Serial.println("Credenciales WiFi borradas");
      });
   }
   server.begin();
}

void cargarServer(int tipoServer){
   if(tipoServer == modoAP){
      server.on("/", [](){
         contenido += "<!DOCTYPE HTML><html><form method='get' action='config'><head><style>html,body{";
         contenido += "width: 100%; height: 100%; margin: 0;  font-family: sans-serif;  } .form{  padding: 30px;  margin: 0 auto; transform: ";
         contenido += "translateY(15%);  border-radius: 4px; max-width: 500px; background-color: #f05f40;  } .input{ width:100%; font-size:30";
         contenido += "px; margin: 0 0 15px 0; border: 1.5px solid #FFF; background-color: transparent;  color: #FFFFFF; border-radius: 5px;";
         contenido += "  } .label{ color: #ffffff; font-size: 30px;  margin: 0 0 5px 0;  height: 300px } .boton{ color: #F05F40; ";
         contenido += "background-color: #FFFFFF;  font-weight: 600; font-size: 3rem;  border: none; width: 100%;  cursor: pointer;  ";
         contenido += "border-radius: 5px; } h1{ text-align: left; color: #F05F40; margin: 0 0 -50px 0;  padding: 30px 0 0 60px; font-size: ";
         contenido += "100px;  } h2{ text-align: center; color: #FFFFFF; margin: 0 auto; } span{ float: right; padding: 0 10% 0 0; font-size: 30px; }</style>  <meta name='viewport' content='";
         contenido += "width=device-width,initial-scale=1'><meta charset='utf-8'>  <title>IOT Device</title> </head> <body>  <h1>EBENTI-IOT</h1";
         contenido += "> <div class='form'>  <h2>Introduzca las credenciales</h2>  <label class='label'>SSID:</label><input class='input' name";
         contenido += "='ssid' maxlength=40> <label class='label'>PASS:</label><input class='input' name='pass' maxlength=40><button class='boton' type='submit'>GUARDAR</button";
         contenido += "> </div>  </body> </form> </html>";
         server.send(200,"text/html",contenido);
      });
      server.on("/config", [](){
         String querySSID = server.arg("ssid");
         String queryPASS = server.arg("pass");

         if(querySSID.length() > 0 && queryPASS.length() > 0){
            Serial.println("Limpiando EEPROM");
            for(int i=0; i<80; i++){EEPROM.write(i,0);}
            Serial.print("SSID: ");
            Serial.println(querySSID);
            Serial.print("PASS: ");
            Serial.println(queryPASS);

            for(int i=0; i< querySSID.length(); i++){
               EEPROM.write(i, querySSID[i]);
               Serial.print(querySSID[i]);
            }
            Serial.print(",");
            for(int i=0; i<queryPASS.length(); i++){
               EEPROM.write(40+i, queryPASS[i]);
               Serial.print(queryPASS[i]);
            }
            Serial.println();

            EEPROM.commit();

            contenido = "{\"Success\":\"datos guardados en la EEPROM, reinicie manualmente el dispositivo\"}";
            codStatus = 200;
         }else{
            contenido = "{\"Error:\":\"error al cargar los datos a la EEPROM\"}";
            codStatus = 404;
         }
         server.send(codStatus,"application/json",contenido);
      });
   }else if(tipoServer == modoWiFi){
      server.on("/",[](){
         server.send(200,"application/json","{\"success:\":\"dispositivo conectado\"}");
         Serial.println("Cliente conectado al dispositivo");
      });
      server.on("/reset_disp",[](){
         server.send(200,"application/json","{\"success:\":\"dispositivo reiniciado\"}");
         Serial.println("Reiniciando dispositivo");
         delay(2000);
         char jsonMensaje[50];
         snprintf(jsonMensaje, 50, "{\"dispositivo\":\"%s\",\"estado\":\"reiniciado\"}", identificadorDispositivo);
         client.publish("sistema/dispositivos/sensores",jsonMensaje);
         delay(1000);
         ESP.restart();
         // HTTPClient http;
         // http.begin(uri_reinicio);
         // http.addHeader("Authorization",token);
         // http.GET();
         // http.end();
         // delay(10);
         // ESP.restart();
      });
      server.on("/hardreset_disp",[](){
         server.send(200,"application/json","{\"success:\":\"dispositivo reseteado\"}");
         delay(2000);
         for(int i=0; i<86; i++){EEPROM.write(i,0);}
         EEPROM.commit();
         Serial.println("EEPROM borrada");
         WiFi.disconnect();
         Serial.println("Credenciales WiFi borradas");
      });
   }
   server.begin();
}

bool configModoAP(){
   WiFi.mode(WIFI_AP);

   WiFi.softAPmacAddress(mac);

   String macDisp = String(mac[WL_MAC_ADDR_LENGTH - 2], HEX) + String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
   macDisp.toUpperCase();

   String nombreAP = "IOT_dev" + macDisp;

   Serial.println(nombreAP);

   char nombreAPChar[nombreAP.length() + 1];

    memset(nombreAPChar, 0, nombreAP.length() + 1);

   for(int i = 0; i < nombreAP.length(); i++){
      nombreAPChar[i] = nombreAP.charAt(i);
   }

   IPAddress ipEstatica(1,1,1,1);
   IPAddress ipMascara(255,255,255,0);

   WiFi.softAPConfig(ipEstatica,ipEstatica,ipMascara);

   const char* passAPchar = "012345678";

   if(WiFi.softAP(nombreAPChar,passAPchar)){
      Serial.println("Modo AP configurado");
      Serial.println("Cargando servidor");
      return true;
   }else{
      Serial.println("Error al configurar el modo AP");
      return false;
   }
}

bool conectar_WiFi(){
   WiFi.softAPdisconnect(true);
   WiFi.mode(WIFI_STA);
   //WiFi.begin(eeprom_wifissid.c_str(),eeprom_wifipass.c_str());
   WiFi.begin(eeprom_ssid,eeprom_pass);

   Serial.println("Intentado conectar a red WiFi");

   int contador_WiFi = 0;

   while(WiFi.status() != WL_CONNECTED && contador_WiFi < 20){
      delay(500);
      Serial.print(".");
      contador_WiFi++;
   }
   Serial.println();

   if(WiFi.status() != WL_CONNECTED){
      Serial.println("Tiempo de conexión WiFi agotado, configurando modo AP");
      if(configModoAP()){
         cargarServer_char(modoAP);
         Serial.println("Modo AP cargado");
         return false;
      }
   }else{
      cargarServer_char(modoWiFi);
      Serial.println("Dispositivo conectado a la red WiFi");
      Serial.println(WiFi.localIP());
      return true;
   }
}

// void configurar_WiFi() {

//   delay(10);
//   // Se configura la conexión al WiFi
//   Serial.println();
//   Serial.print("Conectando a: ");
//   Serial.println(ssid);

//   WiFi.mode(WIFI_STA);
//   WiFi.begin(ssid, password);

//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//   }

//   Serial.println("");
//   Serial.println("WiFi conectado");
//   Serial.println("Direccion IP: ");
//   Serial.println(WiFi.localIP());
// }

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Switch on the LED if an 1 was received as first character
  if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  }

}

void conectar_Servidor() {
  // Se intenta reconectar 
  while (!client.connected()) {
    Serial.println("Intentando conectar con el broker MQTT...");
    // Se intenta la conexióno con el servidor
    if (client.connect(IDcliente)) {
      Serial.println("Dispositio conectado");
      //el dispositivo se conecta y envía el estado al sistma
      char jsonMensaje[50];

      snprintf(jsonMensaje, 50, "{\"dispositivo\":\"%s\",\"estado\":\"activo\"}", identificadorDispositivo);

      client.publish("sistema/dispositivos/sensores",jsonMensaje);
      client.subscribe("sistema/dispositivos/habilitacion");
    } else {
      Serial.print("Falla, rc= ");
      Serial.print(client.state());
      Serial.println(" Se intentará la conexión en 5s");
      // Se esperan los 5 segundos para la intentar reconectar
      delay(5000);
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

  EEPROM.begin(512);
  delay(100);
  Serial.println("");
  leer_EEPROM_char();
  delay(10);
  Serial.println("EEPROM leida");

  // sensorFluido.begin();
  // if(!bme.begin(0x76)){
  //   Serial.println("Error al iniciar el sensor BME, revisar la conexion");
  //   while(1);
  // }
  if(conectar_WiFi()){
    dispositivoConfigurado = true;
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
  }else{
    dispositivoConfigurado = false;
  }
}


float calcularSensacion(float tempF, float humR){
   float temperaturaF = tempF;
   float humedadR = humR;
   float senTermica = -42.379+2.04901523*temperaturaF+10.14333127*humedadR-0.22475541*temperaturaF*humedadR-0.00683783*temperaturaF*temperaturaF-0.05481717*humedadR*humedadR+0.00122874*temperaturaF*temperaturaF*humedadR+0.00085282*temperaturaF*humedadR*humedadR-0.00000199*temperaturaF*temperaturaF*humedadR*humedadR;
   return senTermica;
}

void loop() {
  if(dispositivoConfigurado){
    if(!client.connected()){conectar_Servidor();}

    client.loop();

    long ahora = millis();
    if(ahora - ultimoParpadeo > 500){
      ultimoParpadeo = ahora;
      digitalWrite(pinLed, !digitalRead(pinLed));
    }
    if(ahora - ultimoMensaje > 5000){
      ultimoMensaje = ahora;

      digitalWrite(pinLed, LOW);
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
      char cadenaTopico[100];
      
      char jsonTemAire[50];
      char jsonHumAire[50];
      char jsonSensacion[50];
      char jsonTemFluido[50];
      char jsonPuntoRocio[50];
      char jsonPresionAtm[50];

      snprintf(jsonTemAire, 50, "{\"dispositivo\":\"%s\",\"temperatura\":%.2f}",identificadorDispositivo, valor);
      snprintf(jsonHumAire, 50, "{\"dispositivo\":\"%s\",\"humedad\":%.2f}",identificadorDispositivo, valor);
      snprintf(jsonTemFluido, 50, "{\"dispositivo\":\"%s\",\"temperatura\":%.2f}",identificadorDispositivo, valor);
      snprintf(jsonSensacion, 50, "{\"dispositivo\":\"%s\",\"sensacion\":%.2f}",identificadorDispositivo, valor);
      snprintf(jsonPuntoRocio, 50, "{\"dispositivo\":\"%s\",\"punto_rocio\":%.2f}",identificadorDispositivo, valor);
      snprintf(jsonPresionAtm, 50, "{\"dispositivo\":\"%s\",\"presion\":%.2f}",identificadorDispositivo, valor);

      // snprintf(jsonTemAire, 50, "{\"dispositivo\":\"%s\",\"temperatura\":%.2f}",identificadorDispositivo, temAireC);
      // snprintf(jsonHumAire, 50, "{\"dispositivo\":\"%s\",\"humedad\":%.2f}",identificadorDispositivo, humAire);
      // snprintf(jsonTemFluido, 50, "{\"dispositivo\":\"%s\",\"temperatura\":%.2f}",identificadorDispositivo, temFluido);
      // snprintf(jsonSensacion, 50, "{\"dispositivo\":\"%s\",\"sensacion\":%.2f}",identificadorDispositivo, senAire);
      // snprintf(jsonPuntoRocio, 50, "{\"dispositivo\":\"%s\",\"punto_rocio\":%.2f}",identificadorDispositivo, puntoRocio);
      // snprintf(jsonPresionAtm, 50, "{\"dispositivo\":\"%s\",\"presion\":%.2f}",identificadorDispositivo, presionAtm);

      snprintf(cadenaTopico, 100, "sistema/%s/temAire/sensores",ubicacionDispositivo);
      client.publish(cadenaTopico, jsonTemAire);
      memset(cadenaTopico, 0, sizeof(cadenaTopico));

      snprintf(cadenaTopico, 100, "sistema/%s/humAire/sensores",ubicacionDispositivo);
      client.publish(cadenaTopico, jsonHumAire);
      memset(cadenaTopico, 0, sizeof(cadenaTopico));

      snprintf(cadenaTopico, 100, "sistema/%s/temFluido/sensores",ubicacionDispositivo);
      client.publish(cadenaTopico, jsonTemFluido);
      memset(cadenaTopico, 0, sizeof(cadenaTopico));

      snprintf(cadenaTopico, 100, "sistema/%s/senAire/sensores",ubicacionDispositivo);
      client.publish(cadenaTopico, jsonSensacion);
      memset(cadenaTopico, 0, sizeof(cadenaTopico));

      snprintf(cadenaTopico, 100, "sistema/%s/punRocio/sensores",ubicacionDispositivo);
      client.publish(cadenaTopico, jsonPuntoRocio);
      memset(cadenaTopico, 0, sizeof(cadenaTopico));

      snprintf(cadenaTopico, 100, "sistema/%s/presionAtm/sensores",ubicacionDispositivo);
      client.publish(cadenaTopico, jsonPresionAtm);
      memset(cadenaTopico, 0, sizeof(cadenaTopico));

      digitalWrite(pinTx, LOW);
      digitalWrite(pinLed, HIGH);
    }
  }else{
    long ahora = millis();
    if(ahora - ultimoParpadeo > 120){
      ultimoParpadeo = ahora;
      digitalWrite(pinLed, !digitalRead(pinLed));
    }
  }
  server.handleClient();
}