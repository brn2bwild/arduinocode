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
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <math.h>
#include <time.h>
#include <DFRobot_BME280.h>

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

#define modoAP          0
#define modoWiFi        1
#define modoOK          2

//const char* ssid = "Tesalia-AP";
//const char* password = "aahooraa";

const char* mqtt_server = "192.168.10.200";  //se debe configurar la ip del raspberry para que quede estática
//const char* mqtt_server = "iot.eclipse.org";  //se debe configurar la ip del raspberry para que quede estática
//const char* IDCliente = "keYJw21dQ3g=";   // cadena: stA00001 codificada con algoritmo arcfour en modo CBC clave: sensor
const char* IDCliente = "keYJw21dQ3s=";     // cadena: sTA00002 codificada con algoritmo arcfour en modo CBC clave: sensor
const char* tipoSensor = "sensor02";

//variables utilizadas para conexión con el servidor MQTT
//char cadenaTopico[100]; // variable que contendrá la ruta del topico al cual se subscribirá el dispositivo
char mensaje[50];

//char ubicacionDispositivo[20] = "alebines";
//char IDCliente[20] = "estanque";
//char IDCliente[20] = "cisterna";

long ultimoMensaje = 0; //varible que controlará el tiempo de envío de datos
long ultimoParpadeo = 0;
unsigned char contadorReconexionWiFi = 0;
float valor = 0.0;

char eeprom_ssid[40];
char eeprom_pass[40];
uint8_t MAC_AP[WL_MAC_ADDR_LENGTH];// variable que guardará la MAC_AP del dispositivo
uint8_t MAC_WIFI[6];
uint8_t wifiConfigurado = 0;

String contenido = "";
int codStatus = 0;

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

bool cargarServerAP(){
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
 // else if(tipoServer == modoWiFi){
 //    server.on("/",[](){
 //       server.send(200,"application/json","{\"success:\":\"dispositivo conectado\"}");
 //       Serial.println("Cliente conectado al dispositivo");
 //    });
 //    server.on("/reset_disp",[](){
 //       server.send(200,"application/json","{\"success:\":\"dispositivo reiniciado\"}");
 //       Serial.println("Reiniciando dispositivo");
 //       indicarOrden('r');
 //    });
 //    server.on("/hardreset_disp",[](){
 //       server.send(200,"application/json","{\"success:\":\"dispositivo reseteado\"}");
 //       for(int i=0; i<80; i++){EEPROM.write(i,0);}
 //       EEPROM.commit();
 //       delay(1000);
 //       Serial.println("EEPROM borrada");
 //       WiFi.disconnect();
 //       Serial.println("Credenciales WiFi borradas");
 //    });
 // }
  server.begin();
}

bool configModoAP(){
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
  leer_EEPROM_char();

  delay(10);
  Serial.println("EEPROM leida");

  WiFi.disconnect();
  //WiFi.softAPdisconnect(true);
  //WiFi.begin(eeprom_wifissid.c_str(),eeprom_wifipass.c_str());
  WiFi.begin(eeprom_ssid,eeprom_pass);

  Serial.println("Intentado conectar a red WiFi");

  int contador_WiFi = 0;

  while(WiFi.status() != WL_CONNECTED && contador_WiFi < 30){
    delay(500);
    Serial.print(".");
    contador_WiFi++;
  }
  Serial.println();

  if(WiFi.status() != WL_CONNECTED){
    return false;
  }else{
    return true;
  }

  // if(WiFi.status() != WL_CONNECTED){
  //   Serial.println("Tiempo de conexión WiFi agotado, configurando modo AP");
  //   if(configModoAP()){
  //     cargarServerAP(modoAP);
  //     Serial.println("Modo AP cargado");
  //     return false;
  //   }
  // }else{
  //   cargarServerAP(modoWiFi);
  //   Serial.println("Dispositivo conectado a la red WiFi");
  //   Serial.println(WiFi.localIP());
  //   return true;
  // }
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
  // Switch on the LED if an 1 was received as first character
  //if ((char)payload[0] == '1') {
  //  digitalWrite(BUILTIN_LED, LOW);   // Turn the LED on (Note that LOW is the voltage level
    // but actually the LED is on; this is because
    // it is acive low on the ESP-01)
  //} else {
  //  digitalWrite(BUILTIN_LED, HIGH);  // Turn the LED off by making the voltage HIGH
  //}
}

int conectar_Servidor() {
  // Se intenta reconectar 
  if(WiFi.status() == WL_CONNECTED){ 
    Serial.println("debug 1");
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
        return DISPOSITIVO_CONFIG;
      } else {
        return DISPOSITIVO_NOSERVER;
        // digitalWrite(pinLed, LOW);
        // Serial.print("Falla, rc= ");
        // Serial.print(client.state());
        // Serial.println(" Se intentará la conexión en 5s");
        // delay(200);
        // digitalWrite(pinLed, HIGH);
        // // Se esperan los 5 segundos para la intentar reconectar
        // delay(4000);
      }
    }
  }else{
    return DISPOSITIVO_NOWIFI;
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
   
  // sensorFluido.begin();
  // if(!bme.begin(0x76)){
  //   Serial.println("Error al iniciar el sensor BME, revisar la conexion");
  //   while(1);
  // }
  if(conectar_WiFi()){
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
    wifiConfigurado = modoWiFi;
    Serial.println(WiFi.macAddress());
    WiFi.macAddress(MAC_WIFI);
  }else if(configModoAP()){
    if(cargarServerAP()){
      wifiConfigurado = modoAP;
    }
  }
}


float calcularSensacion(float tempF, float humR){
   float temperaturaF = tempF;
   float humedadR = humR;
   float senTermica = -42.379+2.04901523*temperaturaF+10.14333127*humedadR-0.22475541*temperaturaF*humedadR-0.00683783*temperaturaF*temperaturaF-0.05481717*humedadR*humedadR+0.00122874*temperaturaF*temperaturaF*humedadR+0.00085282*temperaturaF*humedadR*humedadR-0.00000199*temperaturaF*temperaturaF*humedadR*humedadR;
   return senTermica;
}

void loop() {
  long ahora = millis();
  if(wifiConfigurado == modoWiFi){
    Serial.println("debug 2");
    if(!client.connected()){
      Serial.println("debug 3");
      int estadoDispositivo = conectar_Servidor();
      if(estadoDispositivo == DISPOSITIVO_CONFIG){
        wifiConfigurado = modoWiFi;
        Serial.println("debug 4");
      }else if(estadoDispositivo == DISPOSITIVO_NOWIFI){
        if(ahora - ultimoParpadeo > 100){
          Serial.println("debug 5");
          digitalWrite(pinLed, !digitalRead(pinLed));
          if(contadorReconexionWiFi++ > 50){
            Serial.println("debug 6");
            Serial.println("Dispositivo desconectado de la red WiFi");
            contadorReconexionWiFi = 0;
            if(conectar_WiFi()){
              wifiConfigurado == modoWiFi;
            }
          }
        }
      }else if(estadoDispositivo == DISPOSITIVO_NOSERVER){
        while(estadoDispositivo == DISPOSITIVO_NOSERVER){
          Serial.println("debug 7");
          estadoDispositivo = conectar_Servidor(); 
          digitalWrite(pinLed, LOW);
          delay(300);
          digitalWrite(pinLed, HIGH);
          delay(5000);
        }
      }
    }else{
      Serial.println("debug 8");
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

        snprintf(jsonPhFluido, 90, "{\"disp\":\"%s-%02X:%02X:%02X:%02X:%02X:%02X\",\"variable\":\"ph_fluido\",\"valor\":%.2f}",tipoSensor, MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], valor);
        snprintf(jsonTemFluido, 90, "{\"disp\":\"%s-%02X:%02X:%02X:%02X:%02X:%02X\",\"variable\":\"temp_fluido\",\"valor\":%.2f}",tipoSensor, MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], valor);
        snprintf(jsonTemAire, 90, "{\"disp\":\"%s-%02X:%02X:%02X:%02X:%02X:%02X\",\"variable\":\"temp_aire\",\"valor\":%.2f}",tipoSensor, MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], valor);
        snprintf(jsonHumAire, 90, "{\"disp\":\"%s-%02X:%02X:%02X:%02X:%02X:%02X\",\"variable\":\"humedad\",\"valor\":%.2f}",tipoSensor, MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], valor);
        snprintf(jsonSensacion, 90, "{\"disp\":\"%s-%02X:%02X:%02X:%02X:%02X:%02X\",\"variable\":\"sensacion\",\"valor\":%.2f}",tipoSensor, MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], valor);
        snprintf(jsonPuntoRocio, 90, "{\"disp\":\"%s-%02X:%02X:%02X:%02X:%02X:%02X\",\"variable\":\"punto_rocio\",\"valor\":%.2f}",tipoSensor, MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], valor);
        snprintf(jsonPresionAtm, 90, "{\"disp\":\"%s-%02X:%02X:%02X:%02X:%02X:%02X\",\"variable\":\"presion\",\"valor\":%.2f}",tipoSensor, MAC_WIFI[0], MAC_WIFI[1], MAC_WIFI[2], MAC_WIFI[3], MAC_WIFI[4], MAC_WIFI[5], valor);

        // snprintf(jsonTemAire, 90, "{\"dispositivo\":\"%s\",\"temperatura\":%.2f}",IDCliente, temAireC);
        // snprintf(jsonHumAire, 90, "{\"dispositivo\":\"%s\",\"humedad\":%.2f}",IDCliente, humAire);
        // snprintf(jsonTemFluido, 90, "{\"dispositivo\":\"%s\",\"temperatura\":%.2f}",IDCliente, temFluido);
        // snprintf(jsonSensacion, 90, "{\"dispositivo\":\"%s\",\"sensacion\":%.2f}",IDCliente, senAire);
        // snprintf(jsonPuntoRocio, 90, "{\"dispositivo\":\"%s\",\"punto_rocio\":%.2f}",IDCliente, puntoRocio);
        // snprintf(jsonPresionAtm, 90, "{\"dispositivo\":\"%s\",\"presion\":%.2f}",IDCliente, presionAtm);

        client.publish("sistema/phFluido/sensores", jsonPhFluido);
        client.publish("sistema/temFluido/sensores", jsonTemFluido);
        client.publish("sistema/temAire/sensores", jsonTemAire);
        client.publish("sistema/humAire/sensores", jsonHumAire);
        client.publish("sistema/senAire/sensores", jsonSensacion);
        client.publish("sistema/punRocio/sensores", jsonPuntoRocio);
        client.publish("sistema/presionAtm/sensores", jsonPresionAtm);
        digitalWrite(pinTx, LOW);
      }
    }
  }else if(wifiConfigurado == modoAP){
    //if(client.connected()){client.loop();}
    Serial.println("debug 9");
    if(ahora - ultimoParpadeo > 100){
      ultimoParpadeo = ahora;
      Serial.println("debug 10");
      digitalWrite(pinLed, !digitalRead(pinLed));
      if(contadorReconexionWiFi++ > 50){
        Serial.println("debug 11");
        Serial.println("Dispositivo desconectado de la red WiFi");
        contadorReconexionWiFi = 0;
        //if(conectar_WiFi()){wifiConfigurado == modoWiFi;}
      }
    }
  }
  server.handleClient();
}