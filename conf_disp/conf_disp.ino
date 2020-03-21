#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <DHT.h>

#define DHTTYPE DHT22

//credenciales de la red creada por el ESP
const char*	APssid = "IOT_device";
const char* APpass = "0123456789";

//credenciales del host creado en la lap
const int puertoHttp = 80;

//constante de tiempo que establece el intervalo de retraso
//para el envío de los datos del sensor
const unsigned long intervalo = 20000;

const int DHTPin = 0;
const int wifiLed = 2;
const int dataLed = 4;

IPAddress IpEstatica(10,10,10,10);
IPAddress IpMascara(255,255,255,0);

//variables con el contenido de la pagina
String lista;
String contenido;
int codStatus;

//variable que contendrá el array de bytes de la direccion del host
String ehost = "";

//variables que se utilizaran para hacer el retraso de los 20seg
unsigned long prevMilis = 0;

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

	if(esid.length() > 1){
		WiFi.begin(esid.c_str(),epass.c_str());
		if(probarWiFi()){
			WiFi.softAPConfig(IpEstatica,IpEstatica,IpMascara);
			cargarServer(0);
			return;
		}
	}
	setupAP();
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

	lista = "<ol>";
	for (int i = 0; i < redes; ++i){
		lista += "<li>";
		lista += WiFi.SSID(i);
		lista += " (";
		lista += WiFi.RSSI(i);
		lista += ")";
		lista += (WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*";
		lista += "</li>";
	}	

	lista += "</ol>";
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
	if(tipoWeb == 1){
		server.on("/", [](){
			IPAddress ip = WiFi.softAPIP();
			String dirIP = String(ip[0])+'.'+String(ip[1])+'.'+String(ip[2])+'.'+String(ip[3]);
			contenido = "<!DOCTYPE HTML>\r\n<html>Hola desde ";
			contenido += dirIP;
			contenido += "<p>";
			contenido += lista;
			contenido += "</p><form method='get' action='setting'>";
			contenido += "<label>SSID: </label><input name='ssid' length=32><br>";
			contenido += "<label>PASS: </label><input name='pass' length=32><br>";
			contenido += "<label>HOST: </label><input name='host' length=32><br>";
			contenido += "<input type='submit'>";
			contenido += "</form></html>";

			server.send(200,"text/html",contenido);
		});
		server.on("/setting",[](){
			String qsid = server.arg("ssid");
			String qpass = server.arg("pass");
			String qhost = server.arg("host");

			if(qsid.length() > 0 && qpass.length() > 0 && qhost.length() > 0){
				Serial.println("Limpiando EEPROM.");
				for (int i = 0; i < 96; ++i){EEPROM.write(i,0);}
				Serial.print("SSID: ");
				Serial.println(qsid);
				Serial.print("PASS: ");
				Serial.println(qpass);
				Serial.print("HOST: ");
				Serial.println(qhost);

				Serial.println("Escribiendo el SSID en la eeprom:");
				for (int i = 0; i < qsid.length(); ++i){
					EEPROM.write(i,qsid[i]);
					Serial.print("Escrito: ");
					Serial.println(qsid[i]);
				}

				Serial.println("Escribiendo el PASS en la eeprom:");
				for (int i = 0; i < qpass.length(); ++i){
					EEPROM.write(32+i,qpass[i]);
					Serial.print("Escrito: ");
					Serial.println(qpass[i]);
				}

				Serial.println("Escribiendo el HOST en la eeprom:");
				for (int i = 0; i < qhost.length(); ++i){
					EEPROM.write(64+i,qhost[i]);
					Serial.print("Escrito: ");
					Serial.println(qhost[i]);
				}

				EEPROM.commit();

				contenido = "{\"Success\":\"guadado en la eeprom... reinicia el dispositivo para conectar a la nueva red\"}";
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
			server.send(200,"application/json","{\"IP\";\""+dirIP+"\"}");
		});
		server.on("/reset_disp", [](){
			contenido = "<!DOCTYPE HTML>\r\n<html>";
			contenido += "<p>Limpiando la EEPROM</p><br>";
			contenido += "<p>Borrando las credenciales WiFi</p></html>";
			server.send(200,"text/html",contenido);
			Serial.println("Limpiando la EEPROM");
			Serial.println("Borrando credenciales WiFi");
			for (int i = 0; i < 96; ++i){EEPROM.write(i,0);}
			EEPROM.commit();
			WiFi.disconnect();
		});
		server.on("/led1", [](){
			digitalWrite(2, LOW);
			server.send(200,"application/json","{\"led\";\"encendido\"}");
		});
		server.on("/led0", [](){
			digitalWrite(2, HIGH);
			server.send(200,"application/json","{\"led\";\"apagado\"}");
		});
	}
}

void loop(){
	unsigned long actMilis = millis();

	if(actMilis < prevMilis){
		prevMilis = actMilis;
	} else if(actMilis - prevMilis >= intervalo){
		prevMilis = actMilis;
		if(enviarInf() == false){return;}
	} 

	server.handleClient();
}

bool enviarInf(){
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

	String data = "id=sen0002&temp="+String(temp)+"&hum="+String(hum);

	Serial.println("Realizando la peticion:");
	Serial.println(url+"&"+data);
	Serial.println();

	client.print(String("POST ")+url+" HTTP/1.1\r\n"+
					"Host: "+ehost.c_str()+"\r\n"+
					"Accept: */*\r\n"+
					"Content-Length: "+data.length()+"\r\n"+
					"Content-Type: application/x-www-form-urlencoded\r\n"+
					"\r\n"+data);
	//se lee la respuesta enviada por el servidor en ese momento
	delay(50);

	while(client.available()){
		char c = client.read();
		Serial.print(c);
	}

	client.println();
	client.println("Connection: close");
	client.println();
	client.flush();
	client.stop();

	Serial.println("\r\nConexion cerrada");
	return true;
}