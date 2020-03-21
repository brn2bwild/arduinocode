/* IRremoteESP8266: IRsendDemo - demonstrates sending IR codes with IRsend.
 *
 * Version 1.1 January, 2019
 * Based on Ken Shirriff's IrsendDemo Version 0.1 July, 2009,
 * Copyright 2009 Ken Shirriff, http://arcfn.com
 *
 * An IR LED circuit *MUST* be connected to the ESP8266 on a pin
 * as specified by kIrLed below.
 *
 * TL;DR: The IR LED needs to be driven by a transistor for a good result.
 *
 * Suggested circuit:
 *     https://github.com/crankyoldgit/IRremoteESP8266/wiki#ir-sending
 *
 * Common mistakes & tips:
 *   * Don't just connect the IR LED directly to the pin, it won't
 *     have enough current to drive the IR LED effectively.
 *   * Make sure you have the IR LED polarity correct.
 *     See: https://learn.sparkfun.com/tutorials/polarity/diode-and-led-polarity
 *   * Typical digital camera/phones can be used to see if the IR LED is flashed.
 *     Replace the IR LED with a normal LED if you don't have a digital camera
 *     when debugging.
 *   * Avoid using the following pins unless you really know what you are doing:
 *     * Pin 0/D3: Can interfere with the boot/program mode & support circuits.
 *     * Pin 1/TX/TXD0: Any serial transmissions from the ESP8266 will interfere.
 *     * Pin 3/RX/RXD0: Any serial transmissions to the ESP8266 will interfere.
 *   * ESP-01 modules are tricky. We suggest you use a module with more GPIOs
 *     for your first time. e.g. ESP-12 etc.
 */

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* ssid = "Administrativos";
const char* password = "ITSS_ADMINISTRATIVOS@2015";
//const char* ssid = "Tesalia";
//const char* password = "1123581321";
const char* mqtt_server = "192.168.5.51";

const uint16_t kIrLed = 5;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).

IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.

WiFiClient espClient;
PubSubClient client(espClient);

// Example of data captured by IRrecvDumpV2.ino
//uint16_t rawData[67] = {9000, 4500, 650, 550, 650, 1650, 600, 550, 650, 550,
//                        600, 1650, 650, 550, 600, 1650, 650, 1650, 650, 1650,
//                        600, 550, 650, 1650, 650, 1650, 650, 550, 600, 1650,
//                        650, 1650, 650, 550, 650, 550, 650, 1650, 650, 550,
//                        650, 550, 650, 550, 600, 550, 650, 550, 650, 550,
//                        650, 1650, 600, 550, 650, 1650, 650, 1650, 650, 1650,
//                        650, 1650, 650, 1650, 650, 1650, 600};

uint16_t ONOFFrawDataClima1[67] = {9086, 4386,  624, 468,  624, 494,  618, 474,  622, 470,  620, 472,  644, 450,  644, 1612,  622, 1614,  622, 1612,  622, 470,  622, 470,  668, 446,  622, 448,  644, 496,  618, 472,  648, 1588,  622, 470,  646, 470,  620, 470,  622, 1614,  622, 1612,  648, 444,  648, 446,  672, 444,  622, 470,  622, 1614,  648, 1590,  622, 494,  618, 472,  622, 1612,  648, 444,  622, 1614,  648};  // NEC (non-strict) 3811865

uint16_t ONrawDataClima2[229] = {3158, 2988,  3136, 4368,  644, 1582,  644, 470,  642, 1582,  644, 494,  618, 494,  618, 1608,  618, 1584,  616, 522,  618, 494,  642, 1584,  644, 1582,  618, 1608,  618, 494,  644, 472,  642, 1582,  616, 498,  642, 496,  618, 494,  618, 494,  644, 472,  616, 522,  616, 1608,  620, 494,  644, 1554,  644, 474,  638, 496,  592, 520,  644, 470,  642, 496,  618, 494,  616, 498,  616, 496,  642, 474,  640, 1584,  644, 494,  618, 1608,  616, 1588,  614, 520,  618, 496,  616, 1584,  668, 1558,  618, 522,  616, 1586,  640, 496,  616, 496,  640, 472,  644, 466,  644, 496,  616, 496,  616, 496,  642, 498,  616, 496,  620, 494,  642, 470,  644, 468,  620, 522,  616, 496,  618, 1608,  618, 494,  644, 472,  642, 494,  616, 498,  616, 494,  618, 496,  644, 496,  592, 522,  618, 494,  642, 472,  640, 498,  618, 494,  616, 496,  592, 520,  644, 474,  640, 494,  618, 498,  614, 496,  642, 496,  642, 480,  608, 496,  640, 470,  644, 496,  618, 496,  616, 498,  616, 494,  644, 472,  666, 470,  618, 498,  616, 496,  642, 474,  640, 498,  590, 522,  616, 498,  642, 496,  616, 498,  616, 500,  614, 496,  640, 474,  668, 468,  644, 468,  642, 472,  642, 496,  616, 1586,  642, 470,  642, 1584,  616, 496,  642, 1584,  640, 474,  640, 1584,  642, 1584,  640, 496,  616, 1586,  640, 1586,  616};  // HAIER_AC_YRW02
uint16_t OFFrawDataClima2[229] = {3136, 3012,  3136, 4366,  646, 1580,  644, 472,  642, 1584,  642, 496,  616, 496,  618, 1608,  618, 1582,  644, 494,  618, 496,  642, 1584,  642, 1582,  618, 1608,  618, 496,  618, 496,  644, 1580,  618, 498,  644, 494,  618, 494,  642, 470,  618, 498,  640, 496,  616, 1610,  620, 494,  618, 1582,  644, 472,  666, 470,  618, 496,  616, 496,  644, 494,  618, 494,  642, 472,  618, 496,  618, 498,  640, 494,  616, 496,  642, 1584,  642, 1582,  618, 496,  644, 470,  642, 1584,  618, 1608,  618, 496,  616, 1610,  616, 496,  642, 472,  644, 494,  618, 494,  618, 494,  644, 470,  644, 474,  640, 494,  618, 494,  618, 496,  642, 496,  618, 474,  640, 494,  618, 496,  642, 1586,  642, 472,  642, 496,  616, 494,  644, 470,  644, 468,  644, 496,  618, 494,  616, 496,  644, 472,  642, 494,  618, 496,  616, 496,  642, 468,  642, 498,  618, 496,  618, 494,  644, 470,  644, 494,  642, 470,  616, 496,  644, 470,  616, 498,  642, 494,  616, 496,  642, 470,  644, 496,  616, 496,  618, 494,  644, 470,  642, 474,  666, 470,  642, 470,  618, 496,  640, 498,  616, 496,  618, 496,  644, 470,  642, 472,  642, 496,  616, 498,  616, 496,  642, 496,  618, 496,  618, 1608,  618, 494,  618, 1582,  642, 496,  618, 494,  618, 494,  644, 1584,  644, 1582,  618, 496,  642, 1584,  618, 1582,  644};  // HAIER_AC_YRW02
// Example Samsung A/C state captured from IRrecvDumpV2.ino
uint8_t samsungState[kSamsungAcStateLength] = {
    0x02, 0x92, 0x0F, 0x00, 0x00, 0x00, 0xF0,
    0x01, 0xE2, 0xFE, 0x71, 0x40, 0x11, 0xF0};

void callback(char* topic, byte* payload, unsigned int length){
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for(int i = 0; i < length; i++){
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if((char)payload[0] == '1'){
    digitalWrite(2, LOW);
    irsend.sendRaw(ONrawDataClima2, 229, 38);
    delay(100);
    irsend.sendRaw(ONOFFrawDataClima1, 67, 38);
  }else{
    digitalWrite(2, HIGH);
    irsend.sendRaw(OFFrawDataClima2, 229, 38);
    delay(100);
    irsend.sendRaw(ONOFFrawDataClima1, 67, 38);
  }
}

void setup() {
  pinMode(0, INPUT);
  pinMode(4, INPUT);
  pinMode(2, OUTPUT);

  irsend.begin();
#if ESP8266
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
#else  // ESP8266
  Serial.begin(115200, SERIAL_8N1);
#endif  // ESP8266

  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void reconnect(){
  while(!client.connected()){
    Serial.print("Intentando conectar...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if(client.connect(clientId.c_str())){
      Serial.println("conectado");
      client.publish("clima/aulab3/estado", "funcionando");
      client.subscribe("cmnd/aulab3_clima/POWER");
    }else{
      Serial.print("Conexión fallida");
      Serial.print(client.state());
    }
    delay(5000);
  }
}

void loop() {
  if(!client.connected()){
    reconnect();
  }
  client.loop();
  
  if(!digitalRead(0)){
    irsend.sendRaw(ONrawDataClima2, 229, 38);
    delay(100);
    irsend.sendRaw(ONOFFrawDataClima1, 67, 38);
  }
  if(!digitalRead(4)){
    irsend.sendRaw(OFFrawDataClima2, 229, 38);
    delay(100);
    irsend.sendRaw(ONOFFrawDataClima1, 67, 38);
  }
//  Serial.println("NEC");
//  irsend.sendNEC(0x00FFE01FUL);
//  delay(2000);
//  Serial.println("Sony");
//  irsend.sendSony(0xa90, 12, 2);  // 12 bits & 2 repeats
//  delay(2000);
//  Serial.println("a rawData capture from IRrecvDumpV2");
//  irsend.sendRaw(rawData, 67, 38);  // Send a raw data capture at 38kHz.
//  delay(2000);
//  Serial.println("a Samsung A/C state from IRrecvDumpV2");
//  irsend.sendSamsungAC(samsungState);
//  delay(2000);
}
