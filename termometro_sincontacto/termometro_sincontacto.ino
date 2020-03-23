#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include <Adafruit_MLX90614.h>
//#include <DFRobot_BME280.h>

// Include custom images
//#include "images.h"


#define OLED_RESET LED_BUILTIN
Adafruit_SSD1306 display(OLED_RESET);

Adafruit_MLX90614 mlx = Adafruit_MLX90614();

//DFRobot_BME280 bme;

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
  //Serial.begin(9600);
  pinMode(pinBotonArriba, INPUT);
  pinMode(pinBotonAbajo, INPUT);
  pinMode(pinLed, OUTPUT);

  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done
  
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.clearDisplay();
  delay(500);
  
  //Serial.println("Inicializando sensor mlx90614");
  mlx.begin();
  delay(500);
}

//void leerTemperaturas(){
//  //Serial.print("Ambient = "); Serial.print(mlx.readAmbientTempC()); 
//  Serial.print("*C\tObject = "); Serial.print(mlx.readObjectTempC());Serial.println("*C");
//  //Serial.print("Temperatura = "); Serial.print(bme.temperatureValue());
//  //Serial.print("*C\tHumedad = "); Serial.print(bme.humidityValue());Serial.println("%");
//}

//void humedadAire(void) {
//  leerTemperaturas();
//  display.setTextSize(2);
//  display.setTextColor(WHITE);
//  display.setCursor(0,0);
//  display.clearDisplay();
//  display.println("Hum. Aire:");
//  display.setTextSize(3);
//  display.setCursor(5,20);
//  //display.print(bme.humidityValue(),1);
//  display.print((char)247);
//  display.println("%");
//  display.display();
//}
//
//void temperaturaAire(void) {
//
//  leerTemperaturas();
//  display.setTextSize(2);
//  display.setTextColor(WHITE);
//  display.setCursor(0,0);
//  display.clearDisplay();
//  display.println("Tem. Aire:");
//  display.setTextSize(3);
//  display.setCursor(5,20);
//  //display.print(mlx.readAmbientTempC(),1);
//  display.print((char)247);
//  display.println("C");
//  display.display();
//}

void temperaturaObjeto(void) {
  //leerTemperaturas();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.clearDisplay();
  display.println("   Temp:");
  display.setTextSize(3);
  display.setCursor(5,20);
  display.print(mlx.readObjectTempC(),1);
  display.print((char)247);
  display.println("C");
  display.display();
}

Demo demos[] = {temperaturaObjeto};
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
