#include "SPI.h"
#include "nRF24L01.h"
#include "RF24.h"

#define botonPin 	3
#define ledPin		2

RF24 radio(7, 8); // CE, CSN

const byte address = "00001";//0xE8E8F0F0E1LL;

bool estadoBoton = 0;

void setup() {
	pinMode(ledPin, OUTPUT);
	pinMode(botonPin, INPUT_PULLUP);
	radio.begin();
  //radio.setAutoAck(false);
  //radio.setDataRate(RF24_250KBPS);
	radio.openWritingPipe(address);
	radio.stopListening();
}

void loop() {
  estadoBoton = digitalRead(botonPin);
  radio.write(&estadoBoton, sizeof(estadoBoton));
  digitalWrite(ledPin, estadoBoton);
}
