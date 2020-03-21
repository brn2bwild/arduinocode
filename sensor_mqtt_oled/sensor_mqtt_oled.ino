/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 by ThingPulse, Daniel Eichhorn
 * Copyright (c) 2018 by Fabrice Weinberg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * ThingPulse invests considerable time and money to develop these open source libraries.
 * Please support us by buying our products (and not the clones) from
 * https://thingpulse.com
 *
 */

// Include the correct display library
// For a connection via I2C using Wire include
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include <Adafruit_MLX90614.h>
#include <DFRobot_BME280.h>

// Include custom images
//#include "images.h"


#define OLED_RESET LED_BUILTIN
Adafruit_SSD1306 display(OLED_RESET);

Adafruit_MLX90614 mlx = Adafruit_MLX90614();

DFRobot_BME280 bme;

#define pinBotonArriba  14
#define pinBotonAbajo   12
#define pinLed          2

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

typedef void (*Demo)(void);


unsigned long tiempoAnterior = 0;
int demoMode = 0;

const int intervaloLectura = 500;

void setup()   {                
  Serial.begin(115200);
  pinMode(pinBotonArriba, INPUT);
  pinMode(pinBotonAbajo, INPUT);
  pinMode(pinLed, OUTPUT);
  Serial.println("Inicializando sensor mlx90614");

  mlx.begin();
  delay(500);

  if(!bme.begin(0x76)){
    Serial.println("Error al iniciar el sensor BME, revisar la conexion");
    while(1);
  }

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done
  
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.clearDisplay();
  delay(1000);
}

void leerTemperaturas(){
  Serial.print("Ambient = "); Serial.print(mlx.readAmbientTempC()); 
  Serial.print("*C\tObject = "); Serial.print(mlx.readObjectTempC());Serial.println("*C");
  Serial.print("Temperatura = "); Serial.print(bme.temperatureValue());
  Serial.print("*C\tHumedad = "); Serial.print(bme.humidityValue());Serial.println("%");
}

void humedadAire(void) {
  leerTemperaturas();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.println("Hum. Aire:");
  display.setTextSize(3);
  display.setCursor(5,20);
  display.print(bme.humidityValue(),1);
  display.print((char)247);
  display.println("%");
  display.display();
}

void temperaturaAire(void) {

  leerTemperaturas();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.println("Tem. Aire:");
  display.setTextSize(3);
  display.setCursor(5,20);
  display.print(mlx.readAmbientTempC(),1);
  display.print((char)247);
  display.println("C");
  display.display();
}

void temperaturaObjeto(void) {
  leerTemperaturas();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.println("Tem. Obj:");
  display.setTextSize(3);
  display.setCursor(5,20);
  display.print(mlx.readObjectTempC(),1);
  display.print((char)247);
  display.println("C");
  display.display();
}

Demo demos[] = {temperaturaAire, temperaturaObjeto, humedadAire};
int demoLength = (sizeof(demos) / sizeof(Demo));
long timeSinceLastModeSwitch = 0;

void loop() {
  unsigned long ahora = millis();

  if((unsigned long)(ahora - tiempoAnterior) >= intervaloLectura){
    digitalWrite(pinLed, !digitalRead(pinLed));

    display.clearDisplay();  // draw the current demo method
    demos[demoMode]();
    tiempoAnterior = ahora;
  }

  if(!digitalRead(pinBotonArriba)){
    delay(200);
    demoMode = (demoMode + 1) % demoLength;
  }
}