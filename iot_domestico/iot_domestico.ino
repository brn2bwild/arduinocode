#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
//const char* ssid = "HOY_NO";
//const char* pass = "a2h0o1r7a";

ESP8266WebServer server;
MDNSResponder mdns;

const char* ssid = "Tesalia-AP";
const char* pass = "contrasena";
const uint8_t pinLed = 2;

void setup() {
	pinMode(pinLed, OUTPUT);
	Serial.begin(115200); //se inicializa el puerto serial
	delay(10);

	Serial.println();
	Serial.println();
	Serial.print("Conectando con ");
	Serial.println(ssid);

	WiFi.begin(ssid, pass); //se inicializa el objeto que controlará las funciones wifi

	while(WiFi.status() != WL_CONNECTED){
		Serial.print(".");
		digitalWrite(pinLed, 1);
		delay(100);
		digitalWrite(pinLed, 0);
		delay(100);
	}
		
	digitalWrite(pinLed, 1);
	Serial.println();
	Serial.println("WiFi conectado");
	Serial.print("Dirección IP: ");
	Serial.println(WiFi.localIP());
	if(mdns.begin("esp8266",WiFi.localIP())){
		Serial.println("MDNSResponder iniciado...");
	}

	mdns.addService("http","tcp", 80);

	server.on("/",[](){
		IPAddress ip = WiFi.localIP();
		String strIP = String(ip[0])+'.'+String(ip[1])+'.'+String(ip[2])+'.'+String(ip[3]);
		server.send(200,"text/html","Hola desde: " + strIP);
	});
	server.on("/toggle",[](){toggleLED();server.send(200);});
	server.begin();

}

void loop(){
	server.handleClient();
}

void toggleLED(){
	digitalWrite(pinLed,!digitalRead(pinLed));
}
