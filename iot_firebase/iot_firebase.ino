#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <FirebaseArduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// Set these to run example.
#define FIREBASE_HOST   "prueba-9a680.firebaseio.com"
#define FIREBASE_AUTH   "v0QQHiYZEDub3irV9CwUSbk3rR5nAyDeNye9Uohx"
//"AAAAdGHMy1k:APA91bGsMrZSiVb-_Nltp1FbpaDbp6w_bzqe0NTJVRz0XfE0iUhAKyxGgmxRaa6k3xCwU4vPNu9nEfqRFaFqoUKmHAq9ePFdDCM-7AjN9IUV3XM8ErYbEFnSUka3yzIRuiUJzcE1wbMd"
//"AIzaSyDw5F-FmSruA_ElO09vuRb8FsjooPw3UtY"
//"AIzaSyB0dM1mXQz8shEl_NGHtdcJmhtkczY59Qs"
//"v0QQHiYZEDub3irV9CwUSbk3rR5nAyDeNye9Uohx"
#define WIFI_SSID       "Tesalia-AP"
#define WIFI_PASSWORD   "aahooraa"
#define LED             2
#define puertoOne_Wire  0

OneWire oneWire(puertoOne_Wire);

DallasTemperature sensor(&oneWire);

void setup() {
  Serial.begin(9600);
  pinMode(LED, OUTPUT);

  sensor.begin();
  // connect to wifi.
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());
  
  Firebase.begin(FIREBASE_HOST);

  Firebase.setInt("LEDStatus",0);
}

int n = 0;

void loop() {
  
  //Serial.print("Requesting temperature...");
  //sensor.requestTemperatures();
  //Serial.println("HECHO");

  //Serial.print("Temperatura del dispositivo: ");
  //Serial.println(sensor.getTempCByIndex(0));

  //Firebase.setFloat("temp",sensor.getTempCByIndex(0));
  //if(Firebase.failed()){
  //  Serial.print("Error no.:");
  //  Serial.println(Firebase.error());
  //  return;
  //}

  if(Firebase.getInt("LEDStatus")){
    digitalWrite(LED, 0);
  }else{
    digitalWrite(LED, 1);
  }

  if(Firebase.failed()){
   Serial.print("Error:");
   Serial.println(Firebase.error());
   return; 
  }
  delay(1000);

  // set value
  // Firebase.setFloat("number", 42.0);
  // // handle error
  // if (Firebase.failed()) {
  //     Serial.print("setting /number failed:");
  //     Serial.println(Firebase.error());  
  //     return;
  // }
  // delay(1000);
  
  // // update value
  // Firebase.setFloat("number", 43.0);
  // // handle error
  // if (Firebase.failed()) {
  //     Serial.print("setting /number failed:");
  //     Serial.println(Firebase.error());  
  //     return;
  // }
  // delay(1000);

  // // get value 
  // Serial.print("number: ");
  // Serial.println(Firebase.getFloat("number"));
  // delay(1000);

  // // remove value
  // Firebase.remove("number");
  // delay(1000);

  // // set string value
  // Firebase.setString("message", "hello world");
  // // handle error
  // if (Firebase.failed()) {
  //     Serial.print("setting /message failed:");
  //     Serial.println(Firebase.error());  
  //     return;
  // }
  // delay(1000);
  
  // // set bool value
  // Firebase.setBool("truth", false);
  // // handle error
  // if (Firebase.failed()) {
  //     Serial.print("setting /truth failed:");
  //     Serial.println(Firebase.error());  
  //     return;
  // }
  // delay(1000);

  // // append a new value to /logs
  // String name = Firebase.pushInt("logs", n++);
  // // handle error
  // if (Firebase.failed()) {
  //     Serial.print("pushing /logs failed:");
  //     Serial.println(Firebase.error());  
  //     return;
  // }
  // Serial.print("pushed: /logs/");
  // Serial.println(name);
  // delay(1000);
}
