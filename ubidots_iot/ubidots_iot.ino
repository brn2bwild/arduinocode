#include <UbidotsMicroESP8266.h>

#define TOKEN 		"A1E-iWMKRpGpdUD9Ie3hdpnwa2WRc6tWQS"
#define ID1 		"5a567fc5c03f9709b975144a"
#define ID2			"5a569b98c03f972340b64dc4"
//#define WIFISSID	"HOY_NO"
//#define PASSWORD	"a2h0o1r7a"
#define WIFISSID	"Tesalia-AP"
#define PASSWORD	"contrasena"
#define pinLed		2

Ubidots client(TOKEN);

void setup(){
	Serial.begin(115200);
	pinMode(pinLed, OUTPUT);
	delay(10);
	client.wifiConnection(WIFISSID,PASSWORD);
}

void loop(){
	float estado = client.getValue(ID2);
	if(estado == 0.0){
		digitalWrite(pinLed, HIGH);
	}else if(estado == 1.00){
		digitalWrite(pinLed, LOW);
	}
	delay(600);
}