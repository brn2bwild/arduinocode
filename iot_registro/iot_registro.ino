#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <DHT.h>

#define DHTTYPE DHT22

//credenciales de la red creada por el ESP
const char*	APssid = "IOT_device";
const char* APpass = "0123456789";

//credenciales del host creado en la lap
const int puertoHttp = 8080;

//variable para la lectura del json enviado por el servidor
bool lectJson = false;
char jsonArray[45];

//constante de tiempo que establece el intervalo de retraso
//para el envío de los datos del sensor
const unsigned long intervalo = 20000;

const int DHTPin = 0;
const int wifiLed = 2;
const int dataLed = 4;

IPAddress IpEstatica(10,10,10,10);
IPAddress IpMascara(255,255,255,0);

//variables con el contenido de la pagina
//String lista;
String contenido = "";
int codStatus;

//variable que contendrá el array de bytes de la direccion del host
String ehost = "";
String eid = "";

//variables que se utilizaran para hacer el retraso de los 20seg
unsigned long prevMilis = 0;

//variable que guardara el valor de la configuración del dispositivos
bool dispConfig = false;

//puerto donde ser recibiran la peticiones
ESP8266WebServer server;

//se inicializa el objeto del DHT
DHT dht(DHTPin,DHTTYPE);

void setup(){
	iniHardware();
	delay(10);
	
	Serial.println("Leyendo SSID desde la EEPROM...");
	String esid = "";
	for (int i = 0; i < 32; ++i){esid += char(EEPROM.read(i));}
	Serial.print("SSID: ");
	Serial.println(esid);

	Serial.println("Leyendo PASS desde la EEPROM...");
	String epass = "";
	for (int i = 32; i < 64; ++i){epass += char(EEPROM.read(i));}
	Serial.print("PASS: ");
	Serial.println(epass);

	Serial.println("Leyendo HOST desde la EEPROM...");
	//String ehost = "";
	for (int i = 64; i < 96; ++i){ehost	+= char(EEPROM.read(i));}
	Serial.print("HOST: ");
	Serial.println(ehost);

	Serial.println("Leyendo ID desde la EEPROM...");
	//String ehost = "";
	for (int i = 96; i < 101; ++i){eid += char(EEPROM.read(i));}
	Serial.print("ID: ");
	Serial.println(eid);

	if(esid.length() > 1){
		WiFi.softAPdisconnect(true);
		WiFi.begin(esid.c_str(),epass.c_str());
		if(probarWiFi()){
			cargarServer(0);
			if(registroCliente()){dispConfig = true;}
		} else{setupAP();cargarServer(1);}
	}
}

bool registroCliente(){
	WiFiClient client;

	if(!client.connect(ehost.c_str(), puertoHttp)){
		Serial.println("Conexion de registro fallida");
		client.flush();
		client.stop();
		return false;
	}

	Serial.println("Conexion de registro establecida");

	String url = "http://";
			url += String(ehost.c_str());
			url += "/php/agregar.php";

	String data = "ip="+WiFi.localIP().toString()+"&id="+eid+"&mac="+String(WiFi.macAddress());

	Serial.println("Realizando la peticion:");
	Serial.println(url+"?"+data);

	client.print(String("POST ")+url+" HTTP/1.1\r\n"+
					"Host: "+ehost.c_str()+"\r\n"+
					"Accept: */*\r\n"+
					"Content-Length: "+data.length()+"\r\n"+
					"Content-Type: application/x-www-form-urlencoded\r\n"+
					"\r\n"+data);
	//se lee la respuesta enviada por el servidor en ese momento
	delay(80);

	/*while(client.available()){
		char c = client.read();
		if(c == '{'){lectJson = true;}
		if(lectJson){
			if(i < 45){
				jsonArray[i] = c;
				i++;	
			}
			if(c == '}'){
				lectJson = false;
				jsonArray[i] = '\0';
			}
		}
	}*/

	while(client.available()){
		char c = client.read();
		Serial.print(c);
	}

	//String json = String(jsonArray);
	//Serial.print(json);

	client.println();
	client.println("Connection: close");
	client.println();
	client.flush();
	client.stop();

	Serial.println("\r\nConexion cerrada");
	return true;
}

bool probarWiFi(){
	int cont = 0;

	Serial.println("Esperando la conexion WiFi");
	while(cont < 20){
	    if(WiFi.status() == WL_CONNECTED){return true;}
	    delay(500);
	    Serial.print(WiFi.status());
	    cont++;
	}

	Serial.println();
	Serial.println("Tiempo de espera agotado, abriendo configuracion AP");
	return false;
}

void setupAP(){
	WiFi.mode(WIFI_STA);
	WiFi.disconnect();

	delay(100);
	Serial.println("Escaneo hecho...");
	
	int redes = WiFi.scanNetworks();
	if(redes == 0){
		Serial.println("No hay redes encontradas");
	} else {
		Serial.print(redes);
		Serial.println(" redes encontradas");
		for (int i = 0; i < redes; ++i){
			Serial.print(i + 1);
			Serial.print(": ");
			Serial.print(WiFi.SSID(i));
			Serial.print(" (");
			Serial.print(WiFi.RSSI(i));
			Serial.print(")");
			Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ": "*");
			delay(10);
		}
	}

	Serial.println();

	/*lista = "<ol>";
	for (int i = 0; i < redes; ++i){
		lista += "<li class='center-input'>";
		lista += WiFi.SSID(i);
		lista += " (";
		lista += WiFi.RSSI(i);
		lista += ")";
		lista += (WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*";
		lista += "</li>";
	}	
	lista += "</ol>";*/
	
	delay(100);

	WiFi.mode(WIFI_AP);
	uint8_t mac[WL_MAC_ADDR_LENGTH];
//se asigna la mac al dispositivo para inicializarlo en modo AP
	WiFi.softAPmacAddress(mac);
//se obtienen los dos bytes de a
	String macID = String(mac[WL_MAC_ADDR_LENGTH- 2], HEX) + String(mac[WL_MAC_ADDR_LENGTH - 1], HEX);
	macID.toUpperCase();

	String APNameStr = "IOT_Device:" + macID;

	Serial.println(APNameStr);

	char APNameChar[APNameStr.length() + 1];

	memset(APNameChar, 0, APNameStr.length() + 1);

	for(int i = 0; i < APNameStr.length(); i++){
		APNameChar[i] = APNameStr.charAt(i);
	}

	WiFi.softAPConfig(IpEstatica,IpEstatica,IpMascara);

	if(WiFi.softAP(APNameChar,APpass)){
		Serial.println("Iniciando Server por AP");
		cargarServer(1);
		Serial.println("Servidor corriendo...");
	}
}

void iniHardware(){
	Serial.begin(115200);
	EEPROM.begin(512);
	pinMode(wifiLed, OUTPUT);
	
	for (int i = 0; i < 5; ++i){
		digitalWrite(wifiLed, HIGH);
		delay(100);
		digitalWrite(wifiLed, LOW);
		delay(100);
	}
}

void cargarServer(int tipoWeb){
	Serial.println();
	Serial.println("WiFi conectado");
	Serial.print("Local IP: ");
	Serial.println(WiFi.localIP());
	Serial.print("SotfAP IP: ");
	Serial.println(WiFi.softAPIP());
	configServer(tipoWeb);

	//se inicia el servidor
	server.begin();
	Serial.println("Servidor iniciado");
}

void configServer(int tipoWeb){
	if(tipoWeb == 1){ //si la opcion es 1 indica que el servidor esta con el dispositivo en modo AP
		server.on("/", [](){ //si la opcion es 0 indica que el servidor esta con el dispositivo conectado a la red local
			contenido += "<!DOCTYPE HTML><html><form method='get' action='setting'><head><style>";
			contenido += "html,body{width: 100%; height: 100%; margin: 0; font-family: sans-serif; background: linear-gradient(to bottom right, #224870, #44CFCB)}";
			contenido += ".form{padding: 35px; margin: auto; transform: translateY(15%); border-radius: 4px;max-width: 500px;  background-color: #202030}";
			contenido += ".input{width:100%;font-size:30px;margin:0 auto;background-color: rgba(0,0,0,0.0);}";
			contenido += ".label{color: #ffffff;font-size:30px;margin:0 auto;height: 300px}";
			contenido += ".boton{color: #ffffff;background-color: #224870; font-weight: 600; font-size: 3rem; border: 0;width: 100%}";
			contenido += "h1{text-align: center; color: #ffffff; margin: 0 0 6px;}";
			contenido += "input, textarea{color: #ffffff}";
			contenido += "textarea:focus, input:focus{color: #ffffff;}</style><meta name='viewport' content='width=device-width,initial-scale=1'><meta charset='utf-8'>";
			contenido += "<title>IOT Device</title></head><body><div class='form'>";
			contenido += "<h1>Introduzca las credenciales</h1><br>";
			contenido += "<label class='label'>SSID:</label><input class='input' name='ssid' length=32><br>";
			contenido += "<label class='label'>PASS:</label><input class='input' name='pass' length=32><br>";
			contenido += "<label class='label'>HOST:</label><input class='input' name='host' length=32><br>";
			contenido += "<label class='label'>ID:</label><input class='input' name='id' length=32><br><br><br>";
			contenido += "<button class='boton' type='submit'>GUARDAR</button></div></body></form></html>";
			server.send(200,"text/html",contenido);
		});
		server.on("/setting",[](){
			String qsid = server.arg("ssid");
			String qpass = server.arg("pass");
			String qhost = server.arg("host");
			String qid = server.arg("id");

			if(qsid.length() > 0 && qpass.length() > 0 && qhost.length() > 0 && qid.length() > 0){
				Serial.println("Limpiando EEPROM.");
				for (int i = 0; i < 96; ++i){EEPROM.write(i,0);}
				Serial.print("SSID: ");
				Serial.println(qsid);
				Serial.print("PASS: ");
				Serial.println(qpass);
				Serial.print("HOST: ");
				Serial.println(qhost);
				Serial.print("ID: ");
				Serial.println(qid);

				Serial.println("Escribiendo el SSID en la eeprom:");
				for (int i = 0; i < qsid.length(); ++i){
					EEPROM.write(i, qsid[i]);
					Serial.print("Escrito: ");
					Serial.println(qsid[i]);
				}

				Serial.println("Escribiendo el PASS en la eeprom:");
				for (int i = 0; i < qpass.length(); ++i){
					EEPROM.write(32+i, qpass[i]);
					Serial.print("Escrito: ");
					Serial.println(qpass[i]);
				}

				Serial.println("Escribiendo el HOST en la eeprom:");
				for (int i = 0; i < qhost.length(); ++i){
					EEPROM.write(64+i, qhost[i]);
					Serial.print("Escrito: ");
					Serial.println(qhost[i]);
				}

				Serial.println("Escribiendo el ID en la eeprom: ");
				for(int i=0; i< qid.length(); i++){
					EEPROM.write(96+i, qid[i]);
					Serial.print("Escrito: ");
					Serial.println(qid[i]);			    
				}

				EEPROM.commit();

				contenido = "{\"Success\":\"guardado en la eeprom... reinicia el dispositivo para conectar a la nueva red\"}";
				codStatus = 200;
			} else {
				contenido = "{\"Error\":\"404 no encontrado\"}";
				codStatus = 404;
				Serial.println("Enviando 404");
			} 
			server.send(codStatus,"application/json",contenido);
		});
	} else if(tipoWeb == 0){
		server.on("/", [](){
			IPAddress ip = WiFi.localIP();
			String dirIP = String(ip[0])+'.'+String(ip[1])+'.'+String(ip[2])+'.'+String(ip[3]);
			server.send(200,"application/json","{\"IP\":\""+dirIP+"\"}");
		});
		server.on("/reset_disp", [](){
			//contenido = "<!DOCTYPE HTML>\r\n<html>";
			//contenido += "<span style='font-family: sans-serif; font-size: 40px'>EEPROM borrada</span><br>";
			//contenido += "<span style='font-family: sans-serif; font-size: 40px'>Credenciales WiFi borradas</span></html>";
			Serial.println("Limpiando la EEPROM");
			Serial.println("Borrando credenciales WiFi");
			server.send(200,"application/json","{\"EEPROM\":\"borrada\",\"WiFi\":\"borrado\"}");
			for (int i = 0; i < 96; ++i){EEPROM.write(i,0);}
			EEPROM.commit();
			WiFi.disconnect();
		});
		server.on("/toggle", [](){
		  digitalWrite(2, !digitalRead(2));
      if(digitalRead(2)){
			  server.send(200,"application/json","{\"led\":\"encendido\"}");
      }else{
        server.send(200,"application/json","{\"led\":\"apagado\"}");
      }
		});
	}
}

void loop(){
	unsigned long actMilis = millis();

	if(actMilis < prevMilis){
		prevMilis = actMilis;
	} else if(actMilis - prevMilis >= intervalo && dispConfig == true){
		prevMilis = actMilis;
		if(enviarInf() == false){
			Serial.println("Error en el envio de informacion");
		}
	}
	server.handleClient();
}

bool enviarInf(){
	int i = 0;
	float temp = dht.readTemperature();
	float hum = dht.readHumidity();

	if(isnan(temp)||isnan(hum)){
		Serial.println("Error con el sensor");
		return false;
	}

	Serial.println("\r\nLectura hecha");
	Serial.print("Temp: "+String(temp)+"°C ");
	Serial.println("Hum: "+String(hum)+"%");

	Serial.print("Conectando con: ");
	Serial.println(ehost);

	WiFiClient client;

	if(!client.connect(ehost.c_str(),puertoHttp)){
		Serial.println("Conexion fallida");
		client.flush();
		client.stop();
		return false;
	}

	Serial.println("Conexion con el servidor establecida");

	String url = "http://";
			url += String(ehost.c_str());
			url += "/php/cargar.php";

	String data = "id="+eid+"&temp="+String(temp)+"&hum="+String(hum);

	Serial.print("Request: ");
	Serial.println(url+"?"+data);

	client.print(String("POST ")+url+" HTTP/1.1\r\n"+
					"Host: "+ehost.c_str()+"\r\n"+
					"Accept: */*\r\n"+
					"Content-Length: "+data.length()+"\r\n"+
					"Content-Type: application/x-www-form-urlencoded\r\n"+
					"\r\n"+data);
	//se lee la respuesta enviada por el servidor en ese momento
	delay(80);

	memset(jsonArray, 0, sizeof(jsonArray));

	while(client.available()){
		char c = client.read();
		
		if(c == '{'){lectJson = true;}
		if(lectJson){
			if(i < 45){
				jsonArray[i] = c;
				i++;	
			}
			if(c == '}'){
				lectJson = false;
				jsonArray[i] = '\0';
			}
		}
	}

	Serial.print(String(jsonArray));

/*	while(client.available()){
		char c = client.read();
		Serial.print(c);
	    // statement
	}*/

	client.println();
	client.println("Connection: close");
	client.println();
	client.flush();
	client.stop();

	Serial.println("\r\nConexion cerrada");
	return true;
}
