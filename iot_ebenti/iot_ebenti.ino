// Copyright 2015 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

// FirebaseDemo_ESP8266 is a sample that demo the different functions
// of the FirebaseArduino API.

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <FirebaseArduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <DHT.h>

// Set these to run example.
// #define FIREBASE_HOST   "prueba-9a680.firebaseio.com"
// #define FIREBASE_AUTH   "v0QQHiYZEDub3irV9CwUSbk3rR5nAyDeNye9Uohx"
#define WIFI_SSID       "Tesalia-AP"
#define WIFI_PASSWORD   "chimuelito"
#define DHTTYPE         DHT22
// #define WIFI_SSID       "HOY_NO"
// #define WIFI_PASSWORD   "a2h0o1r7a"
#define pinLed          2
#define pinSenFlu       0
#define pinSenAmb       4

OneWire oneWire(pinSenFlu);

DallasTemperature sensorFluido(&oneWire);

DHT sensorAmbiente(pinSenAmb, DHTTYPE);

ESP8266WebServer server;

uint8_t mac[WL_MAC_ADDR_LENGTH];// variable que guardará la MAC del dispositivo

String  folio = ""; //variable que guardará el folio del dispositivo
String  eeprom_wifissid = ""; //variables que guardarán las credenciales WiFi
String  eeprom_wifipass = "";

String  contenido = ""; //variables que guardaran el contenido de la página de configuración
int     codStatus = 0; //así como el status de la respuesta a los clientes

bool    disp_Configurado = false;

void setup() {
  Serial.begin(9600);
  pinMode(pinLed, OUTPUT);

  EEPROM.begin(512);
  delay(100);

  Serial.println();

  for(int i = 0 ; i < 40 ; ++i){ eeprom_wifissid += char(EEPROM.read(i));}
  Serial.print("SSID: ");
  Serial.println(eeprom_wifissid);

  for(int i = 40 ; i < 80 ; ++i){ eeprom_wifipass += char(EEPROM.read(i));}
  Serial.print("PASS: ");
  Serial.println(eeprom_wifipass);

  for(int i = 80 ; i < 85 ; ++i){ folio += char(EEPROM.read(i));}
  Serial.print("FOLIO: ");
  Serial.println(folio);

  delay(10);

  Serial.println("EEPROM leida");

  if(eeprom_wifissid.length() > 1){
    WiFi.softAPdisconnect(true);

    WiFi.begin(eeprom_wifissid.c_str(),eeprom_wifipass.c_str());

    Serial.println("Esperando a conectar WiFi");

    int contador_WiFi = 0;

    while(WiFi.status() != WL_CONNECTED && contador_WiFi < 20){
      delay(500);
      Serial.print(".");
      contador_WiFi++;
    }

    if(WiFi.status() != WL_CONNECTED){
      Serial.println("Tiempo de espera del WiFi agotado, configurando AP");
      if(configModoAP()){
        cargarServer(0);
        Serial.println("Servidor cargado");
        server.begin();
      }
    }else{
      cargarServer(1);
      server.begin();
      Serial.println("WiFi conectado");
      sensorFluido.begin();
      sensorAmbiente.begin();

      Serial.println(WiFi.localIP());

      String payload;

      if(WiFi.status() == WL_CONNECTED){
        HTTPClient http;

        String consulta_recurso = "http://api.ebenti.com/apiconfig.php/rutas?foliorecurso=iot_dispoprod";

        Serial.println(consulta_recurso);
        http.begin(consulta_recurso);
        int httpCode = http.GET();

        if(httpCode > 0){
          payload = http.getString();
          Serial.println(payload);
        }

        DynamicJsonBuffer jBufferRecurso;
        JsonObject& jObjectRecurso = jBufferRecurso.parseObject(payload);
        String uri = jObjectRecurso["uri"];

        uri += "?folio="+folio+"&mac="+String(WiFi.macAddress());
        Serial.println(uri);

        http.begin(uri);
        httpCode = http.GET();

        if(httpCode > 0){
          payload = http.getString();
          Serial.println(payload);
        }
        http.end();

        DynamicJsonBuffer jBufferToken;
        JsonObject& jObjectToken = jBufferToken.parseObject(payload);
        String token = jObjectToken["token"];


        http.begin("http://api.ebenti.com/apiconfig.php/rutas?foliorecurso=iot_dispometadatos");
        httpCode = http.GET();

        if(httpCode > 0){
          payload = http.getString();
          Serial.println(payload);
        }
        http.end();

        DynamicJsonBuffer jBufferMeta;
        JsonObject& jObjectMeta = jBufferMeta.parseObject(payload);
        String uri_meta = jObjectMeta["uri"];
        Serial.println(uri_meta);

        http.begin(uri_meta);
        http.addHeader("Authorization",token);
        //http.addHeader("Authorization","SWZRc25WTjlidHpVR2MwQklaQnU2bDJxZ0xBKy9tOFlOc0ZDcGEyN0tITFZvd0RqNUxRWTM2a2orNEdrZUNzV29rOTcvZlU1WUY3akxnWk9JVWI0UXdzWHFOd0UzT3FGOERhS0FnRzJ6TGxiQ1NSL0lwRHlXS2t0WW9pUGNva0JqRUZXS1JJT0grdXFVRUt3Zmh4ZTltN0ZtbmZzNTFCeU5Mdm4rQWVEeFNLd1p6RHczTlc1U2dHSmVlVGlIUkpxMTJOTXBVWDRhT2tDekVzN0pJNUphODZHZ3ZaWTJUZ1JMN2JHVHhETzgvREtqa2YxL2pJWDdYK2d4NmlYc0ZiZDNvVTFnY3VvNmt1ZlRkcWpSbWs0SGhreTJQVTJkb0I0emRMbGFQbm5HR2RnRjNQTDNiWTM4djBSV0tIVEFLR1g2S1JLSEovWG5CMU9FM2doRzYwaStNc2FRTTl4WmE3RlYzV3hCeWloeUxBeldYTFdST0JnM2EzSllwcmlmTlpaTnR0eVl0V0p2SlIzZHBveFdTRDgzUWxZLzhFSXpYdjlOdGFNM0R6bWVmSHlqenNBWGV0dldUQWpvM1VRc2o4UQ==");
        httpCode = http.GET();

        if(httpCode > 0){
          payload = http.getString();
        }
        http.end();

        Serial.println(payload);
        DynamicJsonBuffer jBufferCredenciales;
        JsonObject& jObjectCredenciales = jBufferCredenciales.parseObject(payload);
        
        if(!jObjectCredenciales.success()){
          Serial.println("Error");
          delay(500);
          return;
        }

        String databaseURL = jObjectCredenciales["databaseURL"];
        databaseURL = databaseURL.substring(8);
        String apiKey = jObjectCredenciales["apiKey"];
        String aux = jObjectCredenciales["folio"];
        folio = aux;
        Serial.println(databaseURL);
        Serial.println(apiKey);

        Firebase.begin(databaseURL);

        if(databaseURL.length() > 0){disp_Configurado = true;}

        if(Firebase.failed()){
          Serial.print("Error no.");
          Serial.println(Firebase.error());
          disp_Configurado = false;
          return;
        }
      } else{
        Serial.println("Error al conectar al WiFi");
        disp_Configurado = false;
        return;
      }
  //Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  //Firebase.setInt("LEDStatus",0);
    }
  }else{
    Serial.println("No hay datos en la EEPROM");
    if(configModoAP()){
      cargarServer(0);
      server.begin();
    }
    Serial.println("Servidor AP cargado");
    disp_Configurado = false;
  }
}

void cargarServer(int tipoServer){
  if(tipoServer == 0){
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
      contenido += "> <span>MAC: " +String(WiFi.macAddress())+"</span> <div class='form'>  <h2>Introduzca las credenciales</h2>  <label class='label'>SSID:</label><input class='input' name";
      contenido += "='ssid' maxlength=40> <label class='label'>PASS:</label><input class='input' name='pass' maxlength=40>  <label class='";
      contenido += "label'>FOLIO:</label><input class='input' name='folio' maxlength=5> <button class='boton' type='submit'>GUARDAR</button";
      contenido += "> </div>  </body> </form> </html>";
      server.send(200,"text/html",contenido);
    });
    server.on("/config", [](){
      String querySSID = server.arg("ssid");
      String queryPASS = server.arg("pass");
      String queryFOLIO = server.arg("folio");

      if(querySSID.length() > 0 && queryPASS.length() > 0 && queryFOLIO.length() > 0){
        Serial.println("Limpiando EEPROM");
        for(int i=0; i<86; i++){EEPROM.write(i,0);}
        Serial.print("SSID: ");
        Serial.println(querySSID);
        Serial.print("PASS: ");
        Serial.println(queryPASS);
        Serial.print("FOLIO: ");
        Serial.println(queryFOLIO);

        for(int i=0; i< querySSID.length(); i++){
          EEPROM.write(i, querySSID[i]);
          Serial.print(querySSID[i]);
        }
        Serial.print(",");
        for(int i=0; i<queryPASS.length(); i++){
          EEPROM.write(40+i, queryPASS[i]);
          Serial.print(queryPASS[i]);
        }
        Serial.print(",");
        for(int i=0; i<queryFOLIO.length(); i++){
          EEPROM.write(80+i, queryFOLIO[i]);
          Serial.print(queryFOLIO[i]);
        }
        Serial.println();

        EEPROM.commit();

        contenido = "{\"Succes\":\"datos guardados en la EEPROM, reinicie el dispositivo\"}";
        codStatus = 200;
      }else{
        contenido = "{\"Error:\":\"error al cargar los datos a la EEPROM\"}";
        codStatus = 404;
      }
      server.send(codStatus,"application/json",contenido);
    });
  }
  else if(tipoServer == 1){
    server.on("/reset_disp",[](){
      Serial.println("Limpiando EEPROM");
      for(int i=0; i<86; i++){EEPROM.write(i,0);}
      EEPROM.commit();
      Serial.println("Borrando las credenciales WiFi");
      WiFi.disconnect();
      server.send(200,"application/json","{\"succes:\":\"dispositivo reseteado\"}");
    });
  }
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
  } else{
    Serial.println("Error al configurar el modo AP");
    return false;
  }
} 

int n = 0;

void loop() {
  // StaticJsonBuffer<200> jsonBuffer;
  // JsonObject& dispositivo = jsonBuffer.createObject();
  // dispositivo["temp"] = n;
  // dispositivo["ph"] = n+2;
  if(disp_Configurado){
    n++;
    sensorFluido.requestTemperatures();
    Serial.print("Temperatura del fluido: ");
    Serial.println(sensorFluido.getTempCByIndex(0));
    Serial.print("Humedad del ambiente: ");
    float humedad = sensorAmbiente.readHumidity();
    Serial.println(humedad);
    Serial.print("Temperatura del ambiente: ");
    float temperatura = sensorAmbiente.readTemperature();
    Serial.println(temperatura);
    Firebase.set("ph/"+folio+"/",n);
    Firebase.setFloat("tempFluido/"+folio+"/",sensorFluido.getTempCByIndex(0));
    Firebase.setFloat("tempAire/"+folio+"/",temperatura);
    Firebase.setFloat("humedadAire/"+folio+"/",humedad);
    if(Firebase.failed()){
      Serial.print("Error no.: ");
      Serial.println(Firebase.error());
      return;
    }
    delay(10000);
  }
  server.handleClient();
}
