#include "SPI.h"
#include "nRF24L01.h"
#include "RF24.h"

#define botonPin 	3
#define ledPin		2

RF24 radio(7, 8); //ce, csn

const byte address[6] = "00001";//0xE8E8F0F0E1LL;
 
bool estadoBoton = 0;

void setup() {
	//pinMode(botonPin, INPUT_PULLUP);
	pinMode(ledPin, OUTPUT);
	//Serial.begin(9600);
	radio.begin();
  //radio.setAutoAck(false);
  //radio.setDataRate(RF24_250KBPS);
	radio.openReadingPipe(1, address);
	radio.startListening();
}

void loop() {
  if(radio.available()){
    radio.read(&estadoBoton, sizeof(estadoBoton));
  }
  digitalWrite(ledPin, estadoBoton);
  
}
