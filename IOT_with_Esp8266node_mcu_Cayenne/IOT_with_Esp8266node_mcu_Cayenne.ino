#include <Adafruit_Sensor.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiType.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>



//#define CAYENNE_DEBUG
#define CAYENNE_PRINT Serial
#include <CayenneMQTTESP8266.h>

char ssid[] = "enter your wifi router name ";
char wifiPassword[] = "enter your wifi router password ";

// Cayenne authentication info. This should be obtained from the Cayenne Dashboard.
char username[] = "enter user name ";
char password[] = "enter password ";
char clientID[] = " enter client id ";

unsigned long lastMillis = 0;
#include "DHT.h"

#define DHTPIN D4     

#define DHTTYPE DHT11  
DHT dht(DHTPIN, DHTTYPE);

float h,t,f,hic,hif;
void setup() {
	Serial.begin(9600);
	Cayenne.begin(username, password, clientID, ssid, wifiPassword);
  pinMode(D0,OUTPUT);
  pinMode(D1,OUTPUT);
  pinMode(D2,OUTPUT);
  pinMode(D3,OUTPUT);
 
}

void loop() {
	Cayenne.loop();

	
	if (millis() - lastMillis > 10000) {
		lastMillis = millis();
  Cayenne.virtualWrite(0, h);
  Cayenne.virtualWrite(1, t);
  Cayenne.virtualWrite(2, hic);
	}
   h = dht.readHumidity();
  
  t = dht.readTemperature();
  
  
   f = dht.readTemperature(true);

  
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  hif = dht.computeHeatIndex(f, h);
  
  hic = dht.computeHeatIndex(t, h, false);

  Serial.print("Humidity: ");
  Serial.print(h);
  
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  
  Serial.print(" *C ");
  Serial.print(f);
  Serial.print(" *F\t");
  Serial.print("Heat index: ");
  Serial.print(hic);
  
  Serial.print(" *C ");
  Serial.print(hif);
  Serial.println(" *F");
  
	
}
CAYENNE_IN(3)
  {
    
    int currentValue=getValue.asInt();
    if(currentValue==1){
      digitalWrite(D0,HIGH);
      }
      else{
       digitalWrite(D0,LOW); 
      }       }
CAYENNE_IN(4)
  {
    
    int currentValue=getValue.asInt();
    if(currentValue==1){
      digitalWrite(D1,HIGH);
      }
      else{
       digitalWrite(D1,LOW); 
      }       }
CAYENNE_IN(5)
  {
    
    int currentValue=getValue.asInt();
    if(currentValue==1){
      digitalWrite(D2,HIGH);
      }
      else{
       digitalWrite(D2,LOW); 
      }       }
 CAYENNE_IN(6)
  {
    
    int currentValue=getValue.asInt();
    if(currentValue==1){
      digitalWrite(D3,HIGH);
      }
      else{
       digitalWrite(D3,LOW); 
      }       }

//Default function for processing actuator commands from the Cayenne Dashboard.
//You can also use functions for specific channels, e.g CAYENNE_IN(1) for channel 1 commands.

