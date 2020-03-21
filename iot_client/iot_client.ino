#include <ESP8266WiFi.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTTYPE DHT22

//const char* ssid = "HOY_NO";
//const char* pass = "a2h0o1r7a";

const char* ssid = "Tesalia-AP";
const char* pass = "vekanek23";

const char* host = "192.168.30.5";

const int DHTPin = 2;

#define wifiLed 	0
#define clientLed 	2
#define dataLed		4

DHT dht(DHTPin,DHTTYPE);

void setup() {
	pinMode(0, OUTPUT);
	pinMode(2, OUTPUT);
	pinMode(4, OUTPUT);
	Serial.begin(115200); //se inicializa el puerto serial
	delay(10);

	Serial.println();
	Serial.println();
	Serial.print("Conectando con ");
	Serial.println(ssid);

	WiFi.begin(ssid, pass); //se inicializa el objeto que controlará las funciones wifi

	

	digitalWrite(wifiLed, 1);
	Serial.println();
	Serial.println("WiFi conectado");
	Serial.print("Dirección IP: ");
	Serial.println(WiFi.localIP());
}

bool ver_wifi(){
	int cont = 0;
	while (cont < 20){
		if(WiFi.status() != WL_CONNECTED){
			return true;
		}
		Serial.print(".");
		digitalWrite(wifiLed, 1);
		delay(100);
		digitalWrite(wifiLed, 0);
		delay(100);
	}
	return false;
}

void loop() {
	delay(20000);
	float temp = dht.readTemperature();
	float hum = dht.readHumidity();

	if(isnan(temp)||isnan(hum)){
		Serial.println("Error en la lectura del sensor");
		return;
	}

	Serial.print("Conectando con: ");
	Serial.println(host);

	//se crea una instancia de wificlient
	WiFiClient client;

	const int Puertohttp = 80;

	if(!client.connect(host,Puertohttp)){
		Serial.println("Conexión fallida");
		return;
	}

	//se crea la dirección que se utilizará para enviar los datos al servidor
	//teniendo en cuenta el nombre del archivo que recibirá los datos
	String url = "http://192.168.1.4/php/cargar.php";
	String data = "id=sen0002&temp=" + String(temp) + "&hum=" + String(hum);

	Serial.print("Realizando la petición: ");
	Serial.println(url);

	client.print(String("POST ") + url + " HTTP/1.1\r\n"+
					"Host: " + host + "\r\n" + 
					"Accept: */*\r\n" +
					"Content-Length: " + data.length() + "\r\n" +
					"Content-Type: application/x-www-form-urlencoded\r\n" +
					"\r\n" + data);

	//se leen todas las lineas enviadas por el servidor para comprobar si
	//se realizó la conexión
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

	Serial.println();
	Serial.println("Conexión cerrada");

}
