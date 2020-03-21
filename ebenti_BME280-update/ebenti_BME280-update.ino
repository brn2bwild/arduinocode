#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <FirebaseArduino.h>
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

#define pinLed          2
#define pinTx           14 // d5
#define pinLecturas     12 // d6
#define pinSenFlu       0  // d3
#define pinSenPh        A0 

#define OffsetPh        0.0

#define modoAP          0
#define modoWiFi        1

OneWire oneWire(pinSenFlu);

DallasTemperature sensorFluido(&oneWire);

DFRobot_BME280 bme;

ESP8266WebServer server;

uint8_t mac[WL_MAC_ADDR_LENGTH];// variable que guardará la MAC del dispositivo

String  folio = "10001"; //variable que guardará el folio del dispositivo
String  eeprom_wifissid = ""; //variables que guardarán las credenciales WiFi
String  eeprom_wifipass = "";

String  contenido = ""; //variables que guardaran el contenido de la página de configuración
int     codStatus = 0; //así como el status de la respuesta a los clientes

int     timezone = -6;

float minFluido= 0.00;
float maxFluido= 0.00;
bool  alertaFluido= 0;
float minAire= 0.00;
float maxAire= 0.00;
bool  alertaAire= 0;
float minHumedad= 0.00;
float maxHumedad= 0.00;
bool  alertaHumedad= 0;
float minSensacion= 0.00;
float maxSensacion= 0.00;
bool  alertaSensacion= 0;
float miniPh= 0.00;
float maxiPh= 0.00;
bool  alertaPh= 0;

String uri_reinicio;
String token;

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
         HTTPClient http;
         http.begin(uri_reinicio);
         http.addHeader("Authorization",token);
         http.GET();
         http.end();
         delay(10);
         ESP.restart();
      });
      server.on("/hardreset_disp",[](){
         server.send(200,"application/json","{\"success:\":\"dispositivo reseteado\"}");
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

bool conectarServidor(){
   String payload;

   if(WiFi.status() == WL_CONNECTED){
      HTTPClient http;

      String consulta_reinicio = "http://api.ebenti.com/apiconfig.php/rutas?foliorecurso=iot_disporeinicio";
      Serial.println(consulta_reinicio);
      http.begin(consulta_reinicio);
      int httpCode = http.GET();
      
      if(httpCode > 0){
         payload = http.getString();
         Serial.println(payload);
         http.end();

         DynamicJsonBuffer jBufferReinicio;
         JsonObject& jObjectReinicio = jBufferReinicio.parseObject(payload);
         String aux_reincio = jObjectReinicio["uri"];
         uri_reinicio = aux_reincio;
         Serial.println(uri_reinicio);

         String consulta_recurso = "http://api.ebenti.com/apiconfig.php/rutas?foliorecurso=iot_dispoprod";

         Serial.println(consulta_recurso);
         http.begin(consulta_recurso);
         httpCode = http.GET();

         if(httpCode > 0){
            payload = http.getString();
            Serial.println(payload);
            http.end();

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
            
               http.end();

               DynamicJsonBuffer jBufferToken;
               JsonObject& jObjectToken = jBufferToken.parseObject(payload);
               String aux_token = jObjectToken["token"];
               token = aux_token;

               http.begin("http://api.ebenti.com/apiconfig.php/rutas?foliorecurso=iot_dispometadatos");
               httpCode = http.GET();

               if(httpCode > 0){
                  payload = http.getString();
                  Serial.println(payload);

                  http.end();

                  DynamicJsonBuffer jBufferMeta;
                  JsonObject& jObjectMeta = jBufferMeta.parseObject(payload);
                  String uri_meta = jObjectMeta["uri"];
                  Serial.println(uri_meta);

                  http.begin(uri_meta);
                  http.addHeader("Authorization",aux_token);
                  httpCode = http.GET();

                  if(httpCode > 0){
                     payload = http.getString();
                     http.end();

                     Serial.println(payload);
                     DynamicJsonBuffer jBufferCredenciales;
                     JsonObject& jObjectCredenciales = jBufferCredenciales.parseObject(payload);
                    
                     if(!jObjectCredenciales.success()){
                        Serial.println("Error al obtener las credenciales");
                        delay(500);
                        ESP.restart();
                     }

                     String databaseURL = jObjectCredenciales["databaseURL"];
                     databaseURL = databaseURL.substring(8);
                     String apiKey = jObjectCredenciales["apiKey"];
                     String minFlu = jObjectCredenciales["tempFluidoMin"];
                     String maxFlu = jObjectCredenciales["tempFluidoMax"];
                     String minAir = jObjectCredenciales["tempAireMin"];
                     String maxAir = jObjectCredenciales["tempAireMax"];
                     String minHum = jObjectCredenciales["humedadAireMin"];
                     String maxHum = jObjectCredenciales["humedadAireMax"];
                     String minSen = jObjectCredenciales["sensacionTermicaMin"];
                     String maxSen = jObjectCredenciales["sensacionTermicaMax"];
                     String minPh = jObjectCredenciales["phMin"];
                     String maxPh = jObjectCredenciales["phMax"];

                     if(minFlu != "null"){
                        minFluido = float(jObjectCredenciales["tempFluidoMin"]);
                        alertaFluido = 1;
                     }
                     if(maxFlu != "null"){
                        maxFluido = float(jObjectCredenciales["tempFluidoMax"]);
                        alertaFluido = 1;
                     }
                     if(minAir != "null"){
                        minAire = float(jObjectCredenciales["tempAireMin"]);
                        alertaAire = 1;
                     }
                     if(maxAir != "null"){
                        maxAire = float(jObjectCredenciales["tempAireMax"]);
                        alertaAire = 1;
                     }
                     if(minHum != "null"){
                        minHumedad = float(jObjectCredenciales["humedadAireMin"]);
                        alertaHumedad = 1;
                     }
                     if(maxHum != "null"){
                        maxHumedad = float(jObjectCredenciales["humedadAireMax"]);
                        alertaHumedad = 1;
                     }
                     if(minSen != "null"){
                        minSensacion = float(jObjectCredenciales["sensacionTermicaMin"]);
                        alertaSensacion = 1;
                     }
                     if(maxSen != "null"){
                        maxSensacion = float(jObjectCredenciales["sensacionTermicaMax"]);
                        alertaSensacion = 1;
                     }
                     if(minPh != "null"){
                        miniPh = float(jObjectCredenciales["phMin"]);
                        alertaPh = 1;
                     }
                     if(maxPh != "null"){
                        maxiPh = float(jObjectCredenciales["phMax"]);
                        alertaPh = 1;
                     }
                 
                     Serial.println(databaseURL);
                     Serial.println(apiKey);

                     Firebase.begin(databaseURL);

                     // if(Firebase.failed()){
                     //    Serial.print("Firebase error: ");
                     //    Serial.println(Firebase.error());
                     //    return false;
                     // }

                     configTime(0, 0, "pool.ntp.org", "time.nist.gov");
                     Serial.println("Configurando hora");
                     while(!time(nullptr)){
                        Serial.print(".");
                        delay(1000);
                     }

                     return true;

                  }else{
                     Serial.println("Error al establecer la conexión con el servidor");
                     return false;
                  }
               }else{
                  Serial.println("Error al establecer la conexión con el servidor");
                  return false;
               }
            }else{
               Serial.println("Error al establecer la conexión con el servidor");
               return false;
            }
         }else{
            Serial.println("Error al establecer la conexión con el servidor");
            return false;
         }
      }else{
         Serial.println("Error al establecer la conexión con el servidor");
         return false;
      }         
   }else{
      Serial.println("WiFi no tiene conexión");
      return false;
   }
}

bool conectarWiFi(){
   WiFi.softAPdisconnect(true);

   WiFi.begin(eeprom_wifissid.c_str(),eeprom_wifipass.c_str());

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
         cargarServer(modoAP);
         Serial.println("Modo AP cargado");
         return false;
      }
   }else{
      cargarServer(modoWiFi);
      Serial.println("Dispositivo conectado a una red WiFi");
      Serial.println(WiFi.localIP());
      return true;
   }
}

void setup(){
   Serial.begin(9600);
   pinMode(pinLed, OUTPUT);
   pinMode(pinTx, OUTPUT);
   pinMode(pinLecturas, OUTPUT);

   digitalWrite(pinLed, LOW);
   digitalWrite(pinTx, LOW);
   digitalWrite(pinLecturas, LOW);

   EEPROM.begin(512);
   delay(100);

   Serial.println();

   for(int i = 0 ; i < 40 ; ++i){ eeprom_wifissid += char(EEPROM.read(i));}
   Serial.print("SSID: ");
   Serial.println(eeprom_wifissid);

   for(int i = 40 ; i < 80 ; ++i){ eeprom_wifipass += char(EEPROM.read(i));}
   Serial.print("PASS: ");
   Serial.println(eeprom_wifipass);

   delay(10);

   Serial.println("EEPROM leida");

   sensorFluido.begin();
   if(!bme.begin(0x76)){
      Serial.println("Ningun sensor encontrado BME, revisar las conexiones");
      while(1);
   }
   if(conectarWiFi()){
      if(conectarServidor()){
         Firebase.setBool("dispositivos/"+folio+"/switch", true);
         if(Firebase.success()){
            Serial.println("Dispositivo conectado y funcionando...");
         }
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
   bool reconectarDispositivo = false;
   
   if(WiFi.status() != WL_CONNECTED){
      Serial.println("Dispositivo desconectado del WiFi");
      reconectarDispositivo = true;
   }

   if(!reconectarDispositivo){
      digitalWrite(pinLecturas,HIGH);
      sensorFluido.requestTemperatures();
      time_t now = time(nullptr);
      Serial.println(now);

      int lecturasPh[10];
      double valorTotalLecturas = 0;

      for(int i=0; i<10; i++){
          lecturasPh[i] = analogRead(pinSenPh);
          delay(10);
      }


      // se ordenan las lecturas al sensor de Ph
      for(int i=0; i<9; i++){
         for(int j=0; j<10; j++){
            if(lecturasPh[i]>lecturasPh[j]){
               int aux = lecturasPh[i];
               lecturasPh[i] = lecturasPh[j];
               lecturasPh[j] = aux;
            }
         }
      }

      for(int i=2; i<8; i++){valorTotalLecturas += lecturasPh[i];}

      valorTotalLecturas = valorTotalLecturas/6;

      float voltajeSensorPh = valorTotalLecturas*3.33/1024.0;
      float valorPh = 3.5*voltajeSensorPh+OffsetPh; // calculando el ph mediante offset
      
      //hacer la calibracióno de los valores de voltaje obtenidos con un voltaje de 3.3V en la alimentación del sensor
      //float valorPh = -5.70 * voltajeSensorPh + 21.338; // calculando el ph mediante pendiente 
      Serial.println(valorTotalLecturas);
      Serial.println(voltajeSensorPh);
      Serial.print(valorPh);
      Serial.println("Ph");
      float temperaturaFluido = sensorFluido.getTempCByIndex(0);
      //Serial.print("Temperatura del fluido: ");
      Serial.print(temperaturaFluido);
      Serial.println("°C");
      float humedadAire = bme.humidityValue();
      //Serial.print("Humedad del ambiente: ");
      Serial.print(humedadAire);
      Serial.println("% HR");
      float temperaturaAC = bme.temperatureValue();
      //Serial.print("Temperatura del ambiente: ");
      Serial.print(temperaturaAC);
      Serial.println("°C");
      float temperaturaAF = temperaturaAC*9.0/5.0+32;
      float sensacion = (calcularSensacion(temperaturaAF, humedadAire)-32)*(5.0/9.0);
      //Serial.print("Sensación termica: ");
      Serial.print(sensacion);
      Serial.println("°C");
      float puntoRocio = temperaturaAC-((100-humedadAire)/5.0);
      //Serial.print("Punto de rocio: ");
      Serial.print(puntoRocio);
      Serial.println("°C");
      float presionAtm = bme.pressureValue() / 100.0F;
      //Serial.print("Presión barométrica: ");
      Serial.print(presionAtm);
      Serial.println(" hPa");
      float altitud = bme.altitudeValue(PRESION_NIVEL_MAR);
      //Serial.print("Altitud aproximada: ");
      Serial.print(altitud);
      Serial.println(" m");
      digitalWrite(pinLecturas, LOW);

      digitalWrite(pinTx, HIGH);
      Firebase.setFloat("tempFluido/"+folio+"/valor/",temperaturaFluido);
      Firebase.set("tempFluido/"+folio+"/hora/",now);
      if(alertaFluido){
         if(temperaturaFluido < minFluido){
            Firebase.set("tempFluido/"+folio+"/alerta/",-1);
         }else if(temperaturaFluido > maxFluido){
            Firebase.set("tempFluido/"+folio+"/alerta/",1);
         }else{
            Firebase.set("tempFluido/"+folio+"/alerta/",0);
         }
      }
      else{
         Firebase.set("tempFluido/"+folio+"/alerta/",0);
      } 

      Firebase.setFloat("tempAire/"+folio+"/valor/",temperaturaAC);
      Firebase.set("tempAire/"+folio+"/hora/",now);
      if(alertaAire){
         if(temperaturaAC < minAire){
            Firebase.set("tempAire/"+folio+"/alerta/",-1);
         }else if(temperaturaAC > maxAire){
            Firebase.set("tempAire/"+folio+"/alerta/",1);
         }else{
            Firebase.set("tempAire/"+folio+"/alerta/",0);
         }
      }else{
         Firebase.set("tempAire/"+folio+"/alerta/",0);
      }

      Firebase.setFloat("humedadAire/"+folio+"/valor/",humedadAire);
      Firebase.set("humedadAire/"+folio+"/hora/",now);
      if(alertaHumedad){      
         Serial.println("debug");
         if(humedadAire < minHumedad){
            Firebase.set("humedadAire/"+folio+"/alerta/",-1);
         }else if(humedadAire > maxHumedad){
            Firebase.set("humedadAire/"+folio+"/alerta/",1);
         }else{
            Firebase.set("humedadAire/"+folio+"/alerta/",0);
         }
      }else{
         Firebase.set("humedadAire/"+folio+"/alerta/",0);
      }

      Firebase.setFloat("sensacionTermica/"+folio+"/valor/",sensacion);
      Firebase.set("sensacionTermica/"+folio+"/hora/",now);
      if(alertaSensacion){
         if(sensacion < minSensacion){
            Firebase.set("sensacionTermica/"+folio+"/alerta/",-1);
         }else if(sensacion > maxSensacion){
            Firebase.set("sensacionTermica/"+folio+"/alerta/",1);
         }else{
            Firebase.set("sensacionTermica/"+folio+"/alerta/",0);
         }
      }else{
         Firebase.set("sensacionTermica/"+folio+"/alerta/",0);
      }

      Firebase.set("ph/"+folio+"/valor/",roundf(valorPh*100)/100);
      Firebase.set("ph/"+folio+"/hora/",now);
      if(alertaPh){
         if(valorPh < miniPh){
            Firebase.set("ph/"+folio+"/alerta/",-1);
         }else if(valorPh > maxiPh){
            Firebase.set("ph/"+folio+"/alerta/",1);
         }else{
            Firebase.set("ph/"+folio+"/alerta/",0);
         }
      }else{
         Firebase.set("ph/"+folio+"/alerta/",0);
      }

      // if(Firebase.failed()){
      //    Serial.print("Firebase error: ");
      //    Serial.println(Firebase.error());
      //    reconectarDispositivo = true;
      // }

      bool estado = Firebase.getBool("dispositivos/"+folio+"/switch");

      if(estado == false){
         HTTPClient http;
         http.begin(uri_reinicio);
         http.addHeader("Authorization",token);
         http.GET();
         http.end();
         delay(10);
         //Serial.println(estado);
         reconectarDispositivo = true;
      }
      digitalWrite(pinTx, LOW);
   }
   if(reconectarDispositivo){
      if(conectarWiFi()){
         if(conectarServidor()){
            reconectarDispositivo = false;
         }else{
            reconectarDispositivo = true;
         }
      }else{
         reconectarDispositivo = true;
      }
   }
   for(int i=0; i<10; i++){
      digitalWrite(pinLed, HIGH);
      delay(250);
      digitalWrite(pinLed, LOW);
      delay(250);
   }
   server.handleClient();
}